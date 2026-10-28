//go:build windows

// PokéLDN Pocket Calm — independent one-click installer for the LILYGO
// T-Dongle S3. Uses Windows native controls (no runtime or pip required).
// Source is offered under the project's AGPL-3.0 license.
package main

import (
	"bytes"
	"context"
	"embed"
	"strconv"
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
	version            = "v1.0 Pocket Calm"
	wmDestroy          = 0x0002
	wmClose            = 0x0010
	wmCommand          = 0x0111
	wmTimer            = 0x0113
	wmPaint            = 0x000F
	wmCtlColorStatic   = 0x0138
	wmEraseBkgnd       = 0x0014
	pbmSetPos          = 0x0402
	psSolid            = 0
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
	initCommonControls = syscall.NewLazyDLL("comctl32.dll").NewProc("InitCommonControls")
	createFont       = gdi32.NewProc("CreateFontW")
	createSolidBrush = gdi32.NewProc("CreateSolidBrush")
	deleteObject     = gdi32.NewProc("DeleteObject")
	setBkMode        = gdi32.NewProc("SetBkMode")
	setTextColor     = gdi32.NewProc("SetTextColor")
	beginPaint       = user32.NewProc("BeginPaint")
	endPaint         = user32.NewProc("EndPaint")
	fillRect         = user32.NewProc("FillRect")
	drawText         = user32.NewProc("DrawTextW")

	mainWindow uintptr
	portBox    uintptr
	logBox     uintptr
	statusBox  uintptr
	flashBtn   uintptr
	refreshBtn uintptr
	progressBar uintptr
	fontBody uintptr
	fontSmall uintptr
	fontTitle uintptr
	brushWhite uintptr
	brushNavy uintptr
	brushCream uintptr
	guiEvents  = make(chan guiEvent, 256)
	flashing   = false
	logText    = ""
)

type rect struct {Left, Top, Right, Bottom int32}
type paintStruct struct {Hdc uintptr; Erase int32; RcPaint rect; Restore int32; IncUpdate int32; Reserved [32]byte}

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
	font := fontBody
	if font == 0 { font, _, _ = getStockObject.Call(defaultGuiFont) }
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
	case wmPaint:
		var ps paintStruct
		hdc, _, _ := beginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		paintBrand(hdc)
		endPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		return 0
	case wmCtlColorStatic:
		hdc := wp
		setBkMode.Call(hdc, 1)
		setTextColor.Call(hdc, 0x00384C62)
		return brushCream
	case wmEraseBkgnd:
		return 1
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
				case "progress":
					if n, err := strconv.Atoi(evt.msg); err == nil { send(progressBar, pbmSetPos, uintptr(n), 0) }
				case "log":
					appendLog(evt.msg)
				case "port":
					setText(portBox, evt.msg)
				case "done":
					flashing = false
					enableWindow.Call(flashBtn, 1)
					enableWindow.Call(refreshBtn, 1)
					setText(statusBox, evt.msg)
					if strings.Contains(evt.msg, "successfully") { send(progressBar,pbmSetPos,100,0) }
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
	// esptool reports transfer progress as percentages on some releases.
	for _, match := range regexp.MustCompile(`\b(\d{1,3})%`).FindAllStringSubmatch(s, -1) {
		if n, err := strconv.Atoi(match[1]); err == nil && n <= 100 { send(progressBar, pbmSetPos, uintptr(10+n*85/100), 0) }
	}
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
	question := "Install Pocket Calm v1.0 on " + port + "?\n\nONLY for the original LILYGO T-Dongle S3 with an integrated 160x80 display.\n\nClose PokéLDN first. This operation replaces the current firmware. Do not unplug during flashing.\n\nContinue?"
	if !confirm(question) {
		return
	}
	flashing = true
	enableWindow.Call(flashBtn, 0)
	enableWindow.Call(refreshBtn, 0)
	send(progressBar, pbmSetPos, 0, 0)
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
	emit("progress", "5")
	emit("status", "Checking ESP32-S3 chip...")
	if err = runTool(program, "--chip", "esp32s3", "--port", port, "flash-id"); err != nil {
		emit("log", "Chip check failed: "+err.Error())
		emit("log", "Check the port, close PokéLDN, or hold BOOT while reconnecting USB.")
		emit("done", "Device check failed. No firmware was written.")
		return
	}
	emit("progress", "10")
	emit("status", "Installing Pocket Calm v1.0 — keep the device connected...")
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
	emit("progress", "100")
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
func makeFont(height, weight int32) uintptr {
	font,_,_ := createFont.Call(uintptr(height),0,0,0,uintptr(weight),0,0,0,0,0,0,0,0,uintptr(unsafe.Pointer(ptr("Segoe UI"))))
	return font
}
func rgb(r,g,b uint8) uintptr { return uintptr(uint32(r)|uint32(g)<<8|uint32(b)<<16) }
func block(hdc uintptr, r rect, brush uintptr) { fillRect.Call(hdc,uintptr(unsafe.Pointer(&r)),brush) }
func paintLabel(hdc uintptr, title string, x,y,w,h int32, font uintptr, color uintptr) {
	setBkMode.Call(hdc,1); setTextColor.Call(hdc,color)
	old,_,_ := gdi32.NewProc("SelectObject").Call(hdc,font)
	r:=rect{x,y,x+w,y+h}
	drawText.Call(hdc,uintptr(unsafe.Pointer(ptr(title))),^uintptr(0),uintptr(unsafe.Pointer(&r)),0)
	gdi32.NewProc("SelectObject").Call(hdc,old)
}
func paintBrand(hdc uintptr) {
	block(hdc,rect{0,0,650,530},brushCream)
	block(hdc,rect{0,0,650,133},brushNavy)
	block(hdc,rect{0,127,650,133},createBrushTemporary(rgb(212,77,75)))
	paintLabel(hdc,"POCKET CALM",26,21,520,45,fontTitle,rgb(243,233,208))
	paintLabel(hdc,"FIRMWARE INSTALLER    •    v1.0",28,78,520,25,fontSmall,rgb(230,184,93))
	paintLabel(hdc,"1   CONNECT YOUR T-DONGLE S3",27,152,575,24,fontSmall,rgb(20,43,70))
	paintLabel(hdc,"2   INSTALL FIRMWARE",27,263,575,24,fontSmall,rgb(20,43,70))
}
func createBrushTemporary(c uintptr) uintptr {
	// Persistent brushes for the window's lifetime; only used for the slim brand stripe.
	if stripeBrush == 0 { stripeBrush,_,_ = createSolidBrush.Call(c) }
	return stripeBrush
}
var stripeBrush uintptr
func main() {
	runtime.LockOSThread()
	initCommonControls.Call()
	fontBody=makeFont(-17,400); fontSmall=makeFont(-16,600); fontTitle=makeFont(-37,700)
	brushCream,_,_=createSolidBrush.Call(rgb(243,233,208))
	brushNavy,_,_=createSolidBrush.Call(rgb(20,43,70))
	brushWhite,_,_=createSolidBrush.Call(rgb(255,255,255))
	instance, _, _ := getModuleHandle.Call(0)
	clsName := ptr("PokeldnPocketCalmInstaller")
	cls := wndClassEx{Size: uint32(unsafe.Sizeof(wndClassEx{})), WndProc: syscall.NewCallback(wndProc), Instance: instance, Background: brushCream, ClassName: clsName}
	if a, _, _ := registerClassEx.Call(uintptr(unsafe.Pointer(&cls))); a == 0 { return }
	hwnd, _, _ := createWindow.Call(0, uintptr(unsafe.Pointer(clsName)), uintptr(unsafe.Pointer(ptr("Pocket Calm — Firmware Installer v1.0"))), uintptr(wsOverlappedWindow&^0x00040000&^0x00010000), uintptr(240), uintptr(140), uintptr(650), uintptr(530), 0, 0, instance, 0)
	if hwnd == 0 { return }
	mainWindow = hwnd
	portBox = makeCtrl("EDIT", "COM5", 27, 188, 118, 35, wsBorder|wsTabStop, idPort)
	refreshBtn = makeCtrl("BUTTON", "DETECT DEVICE", 163, 188, 188, 35, wsTabStop, idDetect)
	flashBtn = makeCtrl("BUTTON", "INSTALL FIRMWARE", 366, 188, 240, 35, wsTabStop, idFlash)
	makeCtrl("STATIC", "Only for the original LILYGO T-Dongle S3 (160 x 80 display)", 27, 229, 590, 23, 0, 0)
	progressBar = makeCtrl("msctls_progress32", "", 27, 297, 578, 19, 0, 107)
	statusBox = makeCtrl("STATIC", "Ready. Close PokéLDN before installing.", 27, 324, 587, 22, 0, idStatus)
	makeCtrl("STATIC", "INSTALLATION LOG", 27, 355, 575, 24, 0, 0)
	logBox = makeCtrl("EDIT", "", 27, 383, 578, 75, wsBorder|wsVScroll|esReadOnly|esMultiline|esAutoVScroll, idLog)
	makeCtrl("STATIC", "Unofficial community project  •  AGPL-3.0  •  Windows 10/11", 27, 468, 580, 21, 0, 0)
	setTimer.Call(mainWindow, 1, 90, 0)
	showWindow.Call(mainWindow, swShow)
	updateWindow.Call(mainWindow)
	go detectPorts()
	var msg msgStruct
	for {
		ret, _, _ := getMessage.Call(uintptr(unsafe.Pointer(&msg)), 0, 0, 0)
		if int32(ret) <= 0 { break }
		translateMessage.Call(uintptr(unsafe.Pointer(&msg)))
		dispatchMessage.Call(uintptr(unsafe.Pointer(&msg)))
	}
}
