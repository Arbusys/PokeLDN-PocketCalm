//go:build windows

// PokéLDN Pocket Calm — independent one-click installer for the LILYGO
// T-Dongle S3. Uses Windows native controls (no runtime or pip required).
// Source is offered under the project's AGPL-3.0 license.
package main

import (
	"bytes"
	"context"
	"embed"
	"fmt"
	"os"
	"os/exec"
	"regexp"
	"runtime"
	"sort"
	"strings"
	"sync"
	"syscall"
	"time"
	"unsafe"
)

//go:embed assets/firmware-merged.bin assets/esptool.exe
var assets embed.FS

const (
	version            = "v0.8 Pocket Calm"
	wmDestroy          = 0x0002
	wmClose            = 0x0010
	wmCommand          = 0x0111
	wmTimer            = 0x0113
	wmSetFont          = 0x0030
	wsOverlappedWindow = 0x00CF0000
	wsVisible          = 0x10000000
	wsChild            = 0x40000000
	wsTabStop          = 0x00010000
	wsVScroll          = 0x00200000
	wsBorder           = 0x00800000
	esReadOnly         = 0x0800
	esMultiline        = 0x0004
	esAutoVScroll      = 0x0040
	bnClicked          = 0
	mbYesNo            = 0x0004
	mbIconWarning      = 0x0030
	idYes              = 6
	idDetect           = 101
	idFlash            = 102
	idPort             = 103
	idLog              = 104
	idStatus           = 105
	swShow             = 5
	colorWindow        = 5
	defaultGuiFont     = 17
)

var (
	user32           = syscall.NewLazyDLL("user32.dll")
	kernel32         = syscall.NewLazyDLL("kernel32.dll")
	gdi32            = syscall.NewLazyDLL("gdi32.dll")
	registerClassEx  = user32.NewProc("RegisterClassExW")
	createWindow     = user32.NewProc("CreateWindowExW")
	defWindowProc    = user32.NewProc("DefWindowProcW")
	getMessage       = user32.NewProc("GetMessageW")
	translateMessage = user32.NewProc("TranslateMessage")
	dispatchMessage  = user32.NewProc("DispatchMessageW")
	showWindow       = user32.NewProc("ShowWindow")
	updateWindow     = user32.NewProc("UpdateWindow")
	messageBox       = user32.NewProc("MessageBoxW")
	postQuitMessage  = user32.NewProc("PostQuitMessage")
	sendMessage      = user32.NewProc("SendMessageW")
	setWindowText    = user32.NewProc("SetWindowTextW")
	getWindowText    = user32.NewProc("GetWindowTextW")
	enableWindow     = user32.NewProc("EnableWindow")
	setTimer         = user32.NewProc("SetTimer")
	getModuleHandle  = kernel32.NewProc("GetModuleHandleW")
	getStockObject   = gdi32.NewProc("GetStockObject")

	mainWindow uintptr
	portBox    uintptr
	logBox     uintptr
	statusBox  uintptr
	flashBtn   uintptr
	refreshBtn uintptr
	guiEvents  = make(chan guiEvent, 256)
	flashing   = false
	logText    = ""
)

type guiEvent struct {
	kind string
	msg  string
}
type point struct{ X, Y int32 }
type msgStruct struct {
	Hwnd     uintptr
	Message  uint32
	_        uint32
	WParam   uintptr
	LParam   uintptr
	Time     uint32
	Pt       point
	LPrivate uint32
	_        uint32
}
type wndClassEx struct {
	Size       uint32
	Style      uint32
	WndProc    uintptr
	ClsExtra   int32
	WndExtra   int32
	Instance   uintptr
	Icon       uintptr
	Cursor     uintptr
	Background uintptr
	MenuName   *uint16
	ClassName  *uint16
	IconSmall  uintptr
}

func ptr(s string) *uint16 { return syscall.StringToUTF16Ptr(s) }
func send(hwnd uintptr, message uint32, wp, lp uintptr) uintptr {
	out, _, _ := sendMessage.Call(hwnd, uintptr(message), wp, lp)
	return out
}
func setText(hwnd uintptr, value string) {
	setWindowText.Call(hwnd, uintptr(unsafe.Pointer(ptr(value))))
}
func makeCtrl(class, caption string, x, y, w, h int32, style uint32, id int) uintptr {
	ret, _, _ := createWindow.Call(0, uintptr(unsafe.Pointer(ptr(class))), uintptr(unsafe.Pointer(ptr(caption))), uintptr(wsChild|wsVisible|style), uintptr(x), uintptr(y), uintptr(w), uintptr(h), mainWindow, uintptr(id), 0, 0)
	font, _, _ := getStockObject.Call(defaultGuiFont)
	send(ret, wmSetFont, font, 1)
	return ret
}
func info(text string) {
	messageBox.Call(mainWindow, uintptr(unsafe.Pointer(ptr(text))), uintptr(unsafe.Pointer(ptr("Pocket Calm Installer"))), 0x40)
}
func confirm(text string) bool {
	res, _, _ := messageBox.Call(mainWindow, uintptr(unsafe.Pointer(ptr(text))), uintptr(unsafe.Pointer(ptr("Confirm firmware installation"))), mbYesNo|mbIconWarning)
	return res == idYes
}

func wndProc(hwnd uintptr, message uint32, wp, lp uintptr) uintptr {
	switch message {
	case wmClose:
		if flashing {
			info("A flash is in progress. Keep this window open until installation finishes.")
			return 0
		}
	case wmDestroy:
		postQuitMessage.Call(0)
		return 0
	case wmCommand:
		id := int(uint16(wp & 0xffff))
		switch id {
		case idDetect:
			if !flashing {
				go detectPorts()
			}
		case idFlash:
			if !flashing {
				startFlash()
			}
		}
		return 0
	case wmTimer:
		for {
			select {
			case evt := <-guiEvents:
				switch evt.kind {
				case "status":
					setText(statusBox, evt.msg)
				case "log":
					appendLog(evt.msg)
				case "port":
					setText(portBox, evt.msg)
				case "done":
					flashing = false
					enableWindow.Call(flashBtn, 1)
					enableWindow.Call(refreshBtn, 1)
					setText(statusBox, evt.msg)
				}
			default:
				return 0
			}
		}
	}
	ret, _, _ := defWindowProc.Call(hwnd, uintptr(message), wp, lp)
	return ret
}
func appendLog(s string) {
	logText += s + "\r\n"
	if len(logText) > 14000 {
		logText = logText[len(logText)-11000:]
	}
	setText(logBox, logText)
	// Scroll to the most recent line.
	send(logBox, 0x00B1 /* EM_SETSEL */, ^uintptr(0), ^uintptr(0))
	send(logBox, 0x00B7 /* EM_SCROLLCARET */, 0, 0)
}
func emit(kind, s string) {
	select {
	case guiEvents <- guiEvent{kind, s}:
	default:
	}
}

var portRegex = regexp.MustCompile(`(?i)\bCOM[0-9]{1,3}\b`)

func validPort(s string) bool { return portRegex.MatchString(s) && portRegex.FindString(s) == s }

func detectPorts() {
	emit("status", "Detecting serial devices...")
	// This is used only to suggest ports; users can still type a COM port.
	script := `Get-CimInstance Win32_PnPEntity | Where-Object {$_.Name -match '\(COM[0-9]+\)'} | ForEach-Object { $_.Name }`
	c, cancel := context.WithTimeout(context.Background(), 14*time.Second)
	defer cancel()
	out, err := exec.CommandContext(c, "powershell.exe", "-NoProfile", "-NonInteractive", "-Command", script).CombinedOutput()
	if err != nil {
		emit("status", "Enter the COM port shown in Windows Device Manager")
		return
	}
	portsMap := map[string]bool{}
	for _, match := range portRegex.FindAllString(string(out), -1) {
		portsMap[strings.ToUpper(match)] = true
	}
	ports := []string{}
	for s := range portsMap {
		ports = append(ports, s)
	}
	sort.Strings(ports)
	switch len(ports) {
	case 0:
		emit("status", "No serial ports found. Enter your COM port manually.")
	case 1:
		emit("port", ports[0])
		emit("status", "Detected "+ports[0]+" — confirm in Device Manager")
	default:
		emit("status", "Detected "+strings.Join(ports, ", ")+". Enter the correct port above.")
		emit("log", "Multiple ports detected: "+strings.Join(ports, ", "))
	}
}
func getPort() string {
	var buf [32]uint16
	getWindowText.Call(portBox, uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)))
	return strings.ToUpper(strings.TrimSpace(syscall.UTF16ToString(buf[:])))
}

func startFlash() {
	port := getPort()
	if !validPort(port) {
		info("Enter a valid COM port, for example COM5.")
		return
	}
	firmware, ferr := assets.ReadFile("assets/firmware-merged.bin")
	esptool, eerr := assets.ReadFile("assets/esptool.exe")
	if ferr != nil || eerr != nil || len(firmware) < 100000 || len(esptool) < 100000 || !bytes.HasPrefix(esptool, []byte{'M', 'Z'}) {
		info("This is a source/development build without bundled firmware or esptool. Download the RELEASE installer generated by GitHub Actions.")
		return
	}
	question := "Install Pocket Calm v0.8 on " + port + "?\n\nONLY for the original LILYGO T-Dongle S3 with an integrated 160x80 display.\n\nClose PokéLDN first. This operation replaces the current firmware. Do not unplug during flashing.\n\nContinue?"
	if !confirm(question) {
		return
	}
	flashing = true
	enableWindow.Call(flashBtn, 0)
	enableWindow.Call(refreshBtn, 0)
	emit("status", "Preparing firmware installation...")
	emit("log", "Preparing device "+port)
	go runFlash(port, firmware, esptool)
}

type lineWriter struct {
	mu  sync.Mutex
	buf []byte
}

func (w *lineWriter) Write(p []byte) (int, error) {
	w.mu.Lock()
	defer w.mu.Unlock()
	n := len(p)
	for _, c := range p {
		if c == '\r' || c == '\n' {
			if len(w.buf) > 0 {
				emit("log", string(bytes.ToValidUTF8(w.buf, []byte("?"))))
				w.buf = w.buf[:0]
			}
		} else if len(w.buf) < 512 {
			w.buf = append(w.buf, c)
		}
	}
	return n, nil
}
func (w *lineWriter) Flush() {
	w.mu.Lock()
	defer w.mu.Unlock()
	if len(w.buf) > 0 {
		emit("log", string(bytes.ToValidUTF8(w.buf, []byte("?"))))
		w.buf = nil
	}
}
func runFlash(port string, firmware, esptool []byte) {
	defer func() {
		if r := recover(); r != nil {
			emit("log", fmt.Sprint("Error: ", r))
			emit("done", "Installation failed.")
		}
	}()
	dir, err := os.MkdirTemp("", "pokeldn-pocketcalm-")
	if err != nil {
		emit("log", "Cannot create temporary folder: "+err.Error())
		emit("done", "Installation failed.")
		return
	}
	defer os.RemoveAll(dir)
	program := dir + `\esptool.exe`
	image := dir + `\firmware.bin`
	if err = os.WriteFile(program, esptool, 0700); err != nil {
		emit("log", err.Error())
		emit("done", "Installation failed.")
		return
	}
	if err = os.WriteFile(image, firmware, 0600); err != nil {
		emit("log", err.Error())
		emit("done", "Installation failed.")
		return
	}
	emit("status", "Checking ESP32-S3 chip...")
	if err = runTool(program, "--chip", "esp32s3", "--port", port, "flash-id"); err != nil {
		emit("log", "Chip check failed: "+err.Error())
		emit("log", "Check the port, close PokéLDN, or hold BOOT while reconnecting USB.")
		emit("done", "Device check failed. No firmware was written.")
		return
	}
	emit("status", "Flashing Pocket Calm v0.8 — do not unplug...")
	if err = runTool(program, "--chip", "esp32s3", "--port", port, "--baud", "460800", "write-flash", "0x0", image); err != nil {
		emit("log", "High-speed flash failed: "+err.Error())
		emit("log", "Retrying once at 115200 baud...")
		emit("status", "Retrying flash at lower speed...")
		if err = runTool(program, "--chip", "esp32s3", "--port", port, "--baud", "115200", "write-flash", "0x0", image); err != nil {
			emit("log", "Flash error: "+err.Error())
			emit("done", "Flash failed — inspect log. Reconnect device if needed.")
			return
		}
	}
	emit("log", "Success. Unplug/reconnect the T-Dongle S3, then start PokéLDN.")
	emit("done", "Firmware installed successfully!")
}
func runTool(program string, args ...string) error {
	c, cancel := context.WithTimeout(context.Background(), 3*time.Minute)
	defer cancel()
	cmd := exec.CommandContext(c, program, args...)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	writer := &lineWriter{}
	cmd.Stdout = writer
	cmd.Stderr = writer
	emit("log", "> esptool "+strings.Join(args[:len(args)-1], " ")+" [firmware if applicable]")
	err := cmd.Run()
	writer.Flush()
	if c.Err() == context.DeadlineExceeded {
		return fmt.Errorf("timed out waiting for device")
	}
	return err
}
func main() {
	runtime.LockOSThread()
	instance, _, _ := getModuleHandle.Call(0)
	clsName := ptr("PokeldnPocketCalmInstaller")
	cls := wndClassEx{Size: uint32(unsafe.Sizeof(wndClassEx{})), WndProc: syscall.NewCallback(wndProc), Instance: instance, Background: uintptr(colorWindow + 1), ClassName: clsName}
	if a, _, _ := registerClassEx.Call(uintptr(unsafe.Pointer(&cls))); a == 0 {
		return
	}
	hwnd, _, _ := createWindow.Call(0, uintptr(unsafe.Pointer(clsName)), uintptr(unsafe.Pointer(ptr("PokéLDN Pocket Calm — Installer"))), uintptr(wsOverlappedWindow&^0x00040000&^0x00010000), uintptr(240), uintptr(180), uintptr(620), uintptr(455), 0, 0, instance, 0)
	if hwnd == 0 {
		return
	}
	mainWindow = hwnd
	makeCtrl("STATIC", "POKÉLDN  /  POCKET CALM  v0.8", 24, 16, 555, 25, 0, 0)
	makeCtrl("STATIC", "Firmware for LILYGO T-Dongle S3 · 160×80 screen · Windows 10/11", 24, 43, 565, 25, 0, 0)
	makeCtrl("STATIC", "USB serial port (check Device Manager):", 24, 81, 340, 20, 0, 0)
	portBox = makeCtrl("EDIT", "COM5", 24, 105, 143, 27, wsBorder|wsTabStop, idPort)
	refreshBtn = makeCtrl("BUTTON", "Detect ports", 181, 102, 121, 32, wsTabStop, idDetect)
	flashBtn = makeCtrl("BUTTON", "Install v0.8", 324, 102, 259, 32, wsTabStop, idFlash)
	makeCtrl("STATIC", "Only supports the original T-Dongle S3. Back up any custom firmware first.", 24, 145, 575, 23, 0, 0)
	statusBox = makeCtrl("STATIC", "Ready — close PokéLDN before flashing.", 24, 173, 570, 25, 0, idStatus)
	logBox = makeCtrl("EDIT", "", 24, 202, 560, 162, wsBorder|wsVScroll|esReadOnly|esMultiline|esAutoVScroll, idLog)
	makeCtrl("STATIC", "Community build · Unofficial · Source available under AGPL-3.0", 24, 374, 570, 22, 0, 0)
	setTimer.Call(mainWindow, 1, 90, 0)
	showWindow.Call(mainWindow, swShow)
	updateWindow.Call(mainWindow)
	go detectPorts()
	var msg msgStruct
	for {
		ret, _, _ := getMessage.Call(uintptr(unsafe.Pointer(&msg)), 0, 0, 0)
		if int32(ret) <= 0 {
			break
		}
		translateMessage.Call(uintptr(unsafe.Pointer(&msg)))
		dispatchMessage.Call(uintptr(unsafe.Pointer(&msg)))
	}
}
