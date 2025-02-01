# Advanced Clipboard Manager

![image](https://github.com/user-attachments/assets/5d5deb02-62e2-43f6-bd65-2f667e96e1c8)

## Overview

The Advanced Clipboard Manager is a Windows-based application that provides enhanced control and diagnostic information about the system clipboard. It allows you to:
- Check the current status of the clipboard.
- Force-unlock it when in use by another process.
- Terminate the process owning the clipboard.
- Copy the clipboard owner process ID to the clipboard.
- Clear the clipboard.
- Preview clipboard data in different formats.

The application offers a modern user interface utilizing common Windows controls and features custom handling of clipboard data formats.

## Features

- **Clipboard Status Check:** Displays the status of the clipboard, including available data formats.
- **Force Unlock:** Attempts to force-close and unlock the clipboard.
- **Process Termination:** Provides an option to terminate the process locking the clipboard.
- **Process Information:** Shows details of the clipboard owner process (PID, process name, window title).
- **Clipboard Preview:** Renders clipboard data in a readable format based on different clipboard formats (text, bitmap, file drops, and more).
- **Auto Refresh:** Automatically refreshes the clipboard status at a 1-second interval.

## Requirements

- A Windows OS with support for Win32 applications.
- Visual Studio (or any compatible C compiler) with Windows SDK.
- Windows libraries including `commctrl.h`, `psapi.h`, and `UxTheme.h`.
- Ensure that the linker is set up to use the Windows subsystem (`/subsystem:windows`) and that `UxTheme.lib` is linked.

## Compilation

To compile the project, you can use the provided build scripts or compile it manually. The following instructions assume you are using Visual Studio or the MSVC command-line tools.

### Using Command Line (MSVC)

1. Open the "Developer Command Prompt for VS".
2. Navigate to the project directory.
3. Compile using the following command:

   ```
   cl /DUNICODE /D_UNICODE clipboard-manager.c /link /subsystem:windows UxTheme.lib comctl32.lib psapi.lib
   ```

### Justfile

If you prefer using [Just](https://just.systems/), a `justfile` is included in the project. Simply run:

   ```
   just
   ```

or for a specific target such as building the project:

   ```
   just build
   ```

### Batch/PowerShell Scripts

Alternatively, you may use the included `run.bat` or `run.ps1` scripts to compile and run the application.

## Usage

1. **Launch the Application:**  
   Double-click the compiled executable or run the provided script to start the Advanced Clipboard Manager.
   
2. **Interface:**
   - **Clipboard Actions (Left Panel):**
     - Use the "Check Clipboard" button to refresh and display current clipboard status.
     - "Force Unlock" attempts to release the clipboard if locked.
     - "Kill Owner Process" terminates the process holding the clipboard (after confirmation).
     - "Copy PID" copies the process ID of the clipboard owner.
     - "Clear Clipboard" empties the clipboard contents.
     - "Auto Refresh" toggles the 1-second auto-refresh for the clipboard status.
   
   - **Clipboard Status (Left Panel):**  
     Displays real-time logs including available formats and detailed information about the clipboard's state.
   
   - **Process Information (Right Panel):**  
     Lists process details such as process ID, name, and window title of the clipboard owner.
   
   - **Clipboard Preview (Right Panel):**  
     Use the provided combo box to select a clipboard format. The preview area displays the clipboard contents in a readable format.

## Code Structure

- **clipboard-manager.c:**  
  Contains the entire implementation of the application, including window creation, event handling, clipboard operations, and UI management.

## Troubleshooting

- **Compilation Errors:**  
  Ensure that the Windows SDK is properly installed and that all required libraries (e.g., `UxTheme.lib`, `comctl32.lib`, `psapi.lib`) are linked.
  
- **Clipboard Access Issues:**  
  The application shows a "Clipboard is locked!" message when another process is using it. Use the "Force Unlock" or "Kill Owner Process" features cautiously.
  
- **UI Scaling/Positioning:**  
  The interface is dynamically scaled based on the window size. If controls appear misaligned, try resizing the window or adjusting system display settings.

## License

MIT License

Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Acknowledgements

- Windows API documentation and examples.
- Community contributions to similar clipboard management projects.
