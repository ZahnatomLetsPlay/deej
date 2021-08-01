package deej

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"math"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/jacobsa/go-serial/serial"
	"go.uber.org/zap"

	"github.com/zahnatomletsplay/deej/util"
)

// SerialIO provides a deej-aware abstraction layer to managing serial I/O
type SerialIO struct {
	comPort  string
	baudRate uint

	deej                  *Deej
	logger                *zap.SugaredLogger
	namedLogger           *zap.SugaredLogger
	stopChannel           chan bool
	connected             bool
	rebooting             bool
	running               bool
	reset                 bool
	configReloadedChannel chan bool
	connOptions           serial.OpenOptions
	conn                  io.ReadWriteCloser
	reader                bufio.Reader
	stopDelay             time.Duration

	savenum                    int
	lastKnownNumSliders        int
	currentSliderPercentValues []float32
	groupnames                 []string
	firstLine                  bool

	sliderMoveConsumers []chan SliderMoveEvent

	returnCommandConsumers []chan string
}

// SliderMoveEvent represents a single slider move captured by deej
type SliderMoveEvent struct {
	SliderID     int
	PercentValue float32
}

var expectedLinePattern = regexp.MustCompile(`^\d{1,4}(\|\d{1,4})*?\r?\n?$`)

// NewSerialIO creates a SerialIO instance that uses the provided deej
// instance's connection info to establish communications with the arduino chip
func NewSerialIO(deej *Deej, logger *zap.SugaredLogger) (*SerialIO, error) {
	logger = logger.Named("serial")

	sio := &SerialIO{
		deej:                   deej,
		logger:                 logger,
		stopChannel:            make(chan bool),
		connected:              false,
		conn:                   nil,
		sliderMoveConsumers:    []chan SliderMoveEvent{},
		running:                false,
		returnCommandConsumers: []chan string{},
		reader:                 bufio.Reader{},
		firstLine:              false,
		reset:                  false,
	}

	logger.Debug("Created serial i/o instance")

	// respond to config changes
	sio.setupOnConfigReload()

	return sio, nil
}

// Initialize Start the Serial Port
func (sio *SerialIO) Initialize() error {

	// don't allow multiple concurrent connections
	if sio.connected {
		sio.logger.Warn("Already connected, can't start another without closing first")
		return errors.New("serial: connection already active")
	}

	// set minimum read size according to platform (0 for windows, 1 for linux)
	// this prevents a rare bug on windows where serial reads get congested,
	// resulting in significant lag
	minimumReadSize := 0
	if util.Linux() {
		minimumReadSize = 1
	}

	sio.connOptions = serial.OpenOptions{
		PortName:        sio.deej.config.ConnectionInfo.COMPort,
		BaudRate:        uint(sio.deej.config.ConnectionInfo.BaudRate),
		DataBits:        8,
		StopBits:        1,
		MinimumReadSize: uint(minimumReadSize),
	}

	sio.stopDelay = 10 * time.Millisecond

	sio.namedLogger = sio.logger.Named(strings.ToLower(sio.connOptions.PortName))

	//Subscribes to config changes for group names

	sio.logger.Debugw("Attempting serial connection",
		"comPort", sio.connOptions.PortName,
		"baudRate", sio.connOptions.BaudRate,
		"minReadSize", minimumReadSize)

	var err error
	sio.conn, err = serial.Open(sio.connOptions)
	if err != nil {
		sio.connected = false
		// might need a user notification here, TBD
		sio.namedLogger.Warnw("Failed to open serial connection", "error", err)
		return fmt.Errorf("open serial connection: %w", err)
	}
	sio.firstLine = false
	sio.reset = false
	sio.connected = true
	sio.namedLogger.Infow("Connected", "conn", sio.conn)

	reader := bufio.NewReader(sio.conn)
	sio.reader = *reader

	sio.WriteVolumePresets(sio.namedLogger)

	sio.logger.Debug(sio.WaitFor(sio.namedLogger, "INITBEGIN"))
	sio.logger.Debug(sio.WaitFor(sio.namedLogger, "INITDONE"))

	sio.rebooting = false

	return nil
}

// Start attempts to connect to our arduino chip
func (sio *SerialIO) Start() error {
	sio.logger.Debug("Starting serial...")
	if sio.connected {
		go func() {
			sio.running = true
			line := ""
			vals := sio.deej.GetSessionMap().getVolumes()
			same := 0
			adjusted := false

			//send group names
			if !sio.WriteGroupNames(sio.namedLogger) {
				sio.running = false
			}

			for sio.running {
				select {
				case <-sio.stopChannel:
					sio.running = false
				default:

					sio.WriteStringLine(sio.namedLogger, "deej.core.values")
					_, line = sio.WaitFor(sio.namedLogger, "values")

					if same > 1 || !sio.firstLine {
						adjusted = sio.handleLine(sio.namedLogger, line)
					}

					if reflect.DeepEqual(vals, sio.deej.GetSessionMap().getVolumes()) || adjusted {
						same++
					} else {
						same = 0
					}

					vals = sio.deej.GetSessionMap().getVolumes()
					if !sio.deej.sessions.refreshing {
						if sio.WriteValues(sio.namedLogger, vals) {
							if !sio.firstLine {
								sio.firstLine = true
							}
						}
					}
				}
			}
		}()
	} else {
		go func() {
			for {
				select {
				case <-sio.stopChannel:
					break
				}
			}
		}()
	}

	return nil
}

// ReadLine read's a line into a channel
func (sio *SerialIO) ReadLine(logger *zap.SugaredLogger) chan string {
	ch := make(chan string)

	go func() {
		for {
			line, err := sio.reader.ReadString('\n')
			//logger.Debugw("Done reading /r")
			//reader.ReadString('\n')
			if err != nil {

				// we probably don't need to log this, it'll happen once and the read loop will stop
				logger.Warnw("Failed to read line from serial", "error", err, "line", line)
				return
			}

			// no reason to log here, just deliver the line to the channel
			ch <- line
		}
	}()

	return ch
}

// IsRunning Returns if the main sync loop is running
func (sio *SerialIO) IsRunning() bool {
	return sio.running
}

// Pause stops active polling for use resume with start
func (sio *SerialIO) Pause() {
	if sio.running {
		sio.stopChannel <- true
	}
}

// Shutdown signals us to shut down our serial connection, if one is active
func (sio *SerialIO) Shutdown() {
	if sio.connected {
		sio.logger.Debug("Shutting down serial connection")

		if sio.running {
			sio.stopChannel <- true
		}

		sio.logger.Debug("Rebooting Arduino")
		sio.rebootArduino(sio.namedLogger)

		sio.close(sio.namedLogger)
		sio.logger.Debug("Serial Shutdown")
	} else {
		sio.logger.Debug("Not currently connected, nothing to stop")
	}
}

func (sio *SerialIO) Restart() {
	if !sio.rebooting {
		sio.rebooting = true
		sio.Shutdown()

		// let the connection close
		// seems kinda pointless though
		<-time.After(sio.stopDelay)

		if inerr := sio.Initialize(); inerr != nil {
			sio.logger.Warnw("Failed to initialize connection", "error", inerr)
		} else {
			if err := sio.Start(); err != nil {
				sio.logger.Warnw("Failed to renew connection", "error", err)
			} else {
				sio.logger.Debug("Renewed connection successfully")
			}
		}
	}
}

func (sio *SerialIO) Reset() {
	go func() {
		sio.reset = true
		sio.Restart()
	}()
}

// SubscribeToSliderMoveEvents returns an unbuffered channel that receives
// a sliderMoveEvent struct every time a slider moves
func (sio *SerialIO) SubscribeToSliderMoveEvents() chan SliderMoveEvent {
	ch := make(chan SliderMoveEvent)
	sio.sliderMoveConsumers = append(sio.sliderMoveConsumers, ch)

	return ch
}

func (sio *SerialIO) rebootArduino(logger *zap.SugaredLogger) {
	if !sio.reset {
		sio.WriteStringLine(sio.namedLogger, "deej.core.reboot")
		_, line := sio.WaitFor(logger, "lastline")
		sio.handleLine(sio.namedLogger, line)
	}
}

// Writes the volume presets from config to
func (sio *SerialIO) WriteVolumePresets(logger *zap.SugaredLogger) bool {
	volumepresets := sio.deej.config.getVolumePresets()
	line := ""
	for pindex, group := range volumepresets {
		logger.Debug(pindex, ", ", group)
	}
	if line != "" {
		return true
	}
	return false
}

// Writes the group names from config to the arduino returns false if nothing was sent
func (sio *SerialIO) WriteGroupNames(logger *zap.SugaredLogger) bool {
	groupnames := sio.deej.config.getGroupNames()
	line := ""
	for index, name := range groupnames {
		if index > len(groupnames)-1 {
			break
		}

		line += name

		if index < len(groupnames)-1 {
			line += "|"
		}
	}
	if line != "" {
		sio.WriteStringLine(logger, "deej.core.receive.groupnames")
		sio.WriteStringLine(logger, line)
		success, rline := sio.WaitFor(sio.namedLogger, line)
		rline = strings.TrimSuffix(rline, "\r")
		if success {
			return true
		} else if !success && rline == "INVALIDCOMMAND" {
			logger.Debug("Command invalid")
			sio.Flush(logger)
			return true
		} else {
			logger.Debug("Received incorrect return ", rline)
			sio.Reset()
			return false
		}
	}
	logger.Debug("Sent nothing ", groupnames, " ", sio.savenum)
	return false
}

// Writes values to the serial port with required command returns false if nothing was sent
func (sio *SerialIO) WriteValues(logger *zap.SugaredLogger, values []float32) bool {
	//go func() {
	line := ""
	if sio.lastKnownNumSliders == 0 {
		sio.lastKnownNumSliders = len(values)
	}
	for index, value := range values {
		if index > sio.lastKnownNumSliders-1 {
			break
		}
		line += strconv.FormatFloat(float64(math.Round(float64(value*1023))), 'f', 0, 32)
		if index < sio.lastKnownNumSliders-1 {
			line += "|"
		}
	}
	//sio.logger.Debug("Sending values:", line)
	if line != "" {
		sio.WriteStringLine(logger, "deej.core.receive")
		sio.WriteStringLine(logger, line)
		success, rline := sio.WaitFor(sio.namedLogger, line)
		rline = strings.TrimSuffix(rline, "\r")
		if success {
			return true
		} else if !success && rline == "INVALIDCOMMAND" {
			logger.Debug("Command invalid")
			sio.Flush(logger)
			return false
		} else {
			logger.Debug("Received incorrect return ", rline)
			sio.Reset()
			return false
		}
	}

	logger.Debug("Couldn't send values:", line)

	return false
	//}()
}

// WriteStringLine retruns nothing
// Writes a string to the serial port
func (sio *SerialIO) WriteStringLine(logger *zap.SugaredLogger, line string) {
	_, err := sio.conn.Write([]byte((line + "\r\n")))

	if err != nil {

		logger.Debug("Failed to write line to serial", "error", err, "line", line)
		sio.Reset()
		return
	}
}

// WriteBytesLine retruns nothing
// Writes a byteArray to the serial port
func (sio *SerialIO) WriteBytesLine(logger *zap.SugaredLogger, line []byte) {
	_, err := sio.conn.Write([]byte(line))
	if err != nil {

		// we probably don't need to log this, it'll happen once and the read loop will stop
		// logger.Warnw("Failed to read line from serial", "error", err, "line", line)
		return
	}
	_, err = sio.conn.Write([]byte("\r\n"))

	if err != nil {

		// we probably don't need to log this, it'll happen once and the read loop will stop
		// logger.Warnw("Failed to read line from serial", "error", err, "line", line)
		sio.Reset()
		return
	}
}

// WriteBytes retruns nothing
// Writes a byteArray to the serial port
func (sio *SerialIO) WriteBytes(logger *zap.SugaredLogger, line []byte) {
	_, err := sio.conn.Write([]byte(line))
	if err != nil {

		// we probably don't need to log this, it'll happen once and the read loop will stop
		// logger.Warnw("Failed to read line from serial", "error", err, "line", line)
		sio.Reset()
		return
	}
}

// WaitFor returns if the received string equals the given string
// Waits for the specified line befor continueing
func (sio *SerialIO) WaitFor(logger *zap.SugaredLogger, cmdKey string) (success bool, value string) {

	//logger.Debug("Waiting for ", cmdKey)
	reader := sio.reader

	line, err := reader.ReadString('\r')

	if err != nil {
		sio.logger.Error("Error reading line", "Error: ", err, "Line: ", line)
		sio.Reset()
		return false, ""
	}

	if len(line) > 0 {

		if line == cmdKey+"\r" {
			return true, line
		}

		return false, line
	}

	return false, ""
}

// Flush clears out the buffers of the serial port
func (sio *SerialIO) Flush(logger *zap.SugaredLogger) {
	go func() {
		sio.WriteStringLine(logger, "deej.core.flush")
	}()
	lineChannel := sio.ReadLine(logger)
loop:
	for {
		select {
		case <-lineChannel:
		case <-time.After(100 * time.Millisecond):
			break loop
		}

	}
	lineChannel = nil
}

func (sio *SerialIO) setupOnConfigReload() {
	configReloadedChannel := sio.deej.config.SubscribeToChanges()
	//sessionReloadChannel := sio.deej.sessions.SubscribeToSessionReload()

	go func() {
		for {
			select {
			case <-configReloadedChannel:
				sio.logger.Debug("Detected config reload ", sio.configReloadedChannel)
				// make any config reload unset our slider number to ensure process volumes are being re-set
				// (the next read line will emit SliderMoveEvent instances for all sliders)\
				// this needs to happen after a small delay, because the session map will also re-acquire sessions
				// whenever the config file is reloaded, and we don't want it to receive these move events while the map
				// is still cleared. this is kind of ugly, but shouldn't cause any issues
				go func() {
					sio.savenum = sio.lastKnownNumSliders
					<-time.After(sio.stopDelay)
					sio.lastKnownNumSliders = 0
				}()

				// if connection params have changed, attempt to stop and start the connection
				if sio.deej.config.ConnectionInfo.COMPort != sio.connOptions.PortName ||
					uint(sio.deej.config.ConnectionInfo.BaudRate) != sio.connOptions.BaudRate ||
					!reflect.DeepEqual(sio.deej.config.GroupNames, sio.groupnames) {

					sio.logger.Info("Detected change in connection parameters, attempting to renew connection")
					sio.Restart()
				}
			}
		}
	}()
}

func (sio *SerialIO) close(logger *zap.SugaredLogger) {
	if err := sio.conn.Close(); err != nil {
		logger.Warnw("Failed to close serial connection", "error", err)
	} else {
		logger.Debug("Serial connection closed")
	}

	sio.conn = nil
	sio.connected = false
}

//Returns true if volume was adjusted
func (sio *SerialIO) handleLine(logger *zap.SugaredLogger, line string) bool {

	// trim the suffix
	line = strings.TrimSuffix(line, "\r")

	// this function receives an unsanitized line which is guaranteed to end with LF,
	// but most lines will end with CRLF. it may also have garbage instead of
	// deej-formatted values, so we must check for that! just ignore bad ones
	if !expectedLinePattern.MatchString(line) {
		logger.Info("Unexpected line pattern", line)
		if line == "INITDONE" || line == "INITBEGIN" {
			logger.Info("Arduino reset detected, restarting serial connection")
			sio.Reset()
		}
		return false
	}

	// split on pipe (|), this gives a slice of numerical strings between "0" and "1023"
	splitValues := strings.Split(line, "|")

	numSliders := len(splitValues)

	// update our slider count, if needed - this will send slider move events for all
	if numSliders != sio.lastKnownNumSliders {
		logger.Infow("Detected sliders", "amount", numSliders)
		sio.lastKnownNumSliders = numSliders
		sio.savenum = numSliders
		sio.currentSliderPercentValues = make([]float32, numSliders)

		// reset everything to be an impossible value to force the slider move event later
		for idx := range sio.currentSliderPercentValues {
			sio.currentSliderPercentValues[idx] = -1.0
		}
	}

	// for each slider:
	moveEvents := []SliderMoveEvent{}
	for sliderIdx, stringValue := range splitValues {

		// convert string values to integers ("1023" -> 1023)
		number, _ := strconv.Atoi(stringValue)

		// turns out the first line could come out dirty sometimes (i.e. "4558|925|41|643|220")
		// so let's check the first number for correctness just in case
		if sliderIdx == 0 && number > 1023 {
			sio.logger.Debugw("Got malformed line from serial, ignoring", "line", line)
			return false
		}

		// map the value from raw to a "dirty" float between 0 and 1 (e.g. 0.15451...)
		dirtyFloat := float32(number) / 1023.0

		// normalize it to an actual volume scalar between 0.0 and 1.0 with 2 points of precision
		normalizedScalar := util.NormalizeScalar(dirtyFloat)

		// if sliders are inverted, take the complement of 1.0
		if sio.deej.config.InvertSliders {
			normalizedScalar = 1 - normalizedScalar
		}

		//logger.Debug(sio.currentSliderPercentValues[sliderIdx], " ", normalizedScalar, " ", number)

		// check if it changes the desired state (could just be a jumpy raw slider value)
		if util.SignificantlyDifferent(sio.currentSliderPercentValues[sliderIdx], normalizedScalar, sio.deej.config.NoiseReductionLevel) {

			// if it does, update the saved value and create a move event
			if sio.firstLine {
				sio.currentSliderPercentValues[sliderIdx] = normalizedScalar
			} else {
				sio.currentSliderPercentValues[sliderIdx] = -10
			}

			moveEvents = append(moveEvents, SliderMoveEvent{
				SliderID:     sliderIdx,
				PercentValue: normalizedScalar,
			})

			if sio.deej.Verbose() {
				logger.Debugw("Slider moved", "event", moveEvents[len(moveEvents)-1])
			}
		}
	}

	// deliver move events if there are any, towards all potential consumers
	// don't trigger events before arduino has adjusted to computer's values
	if len(moveEvents) > 0 && sio.firstLine {
		for _, consumer := range sio.sliderMoveConsumers {
			for _, moveEvent := range moveEvents {
				consumer <- moveEvent
			}
		}
	}
	if len(moveEvents) > 0 {
		return true
	} else {
		return false
	}
}
