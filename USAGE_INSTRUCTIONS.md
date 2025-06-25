# HybridCAD Usage Instructions

## Fixed Issues and New Features

### 1. Grid Toggle is Now Working
- Press **G** to toggle the grid on/off
- You can also use the menu: View → Grid or the toolbar button
- The grid now properly renders on different planes (XY, XZ, YZ)

### 2. Camera Movement (WASD Controls)
- **W** - Move camera forward
- **S** - Move camera backward  
- **A** - Move camera left
- **D** - Move camera right
- **Q** - Move camera down
- **E** - Move camera up

The camera movement is now smooth and continuous while keys are held down.

### 3. Shape Placement
- Press **P** to enter shape placement mode
- Click once to set the start point
- Move mouse and click again to set the end point
- Right-click or press **Escape** to cancel placement

### 4. Line Sketching (New Feature!)
- Press **L** to start line sketching
- Click to set start point, move mouse, click again to set end point
- Lines are drawn in real-time as you move the mouse

### 5. Rectangle Sketching (New Feature!)
- Press **R** to start rectangle sketching  
- Click to set first corner, move mouse, click to set opposite corner
- Rectangle preview is shown in real-time

### 6. Circle Sketching (New Feature!)
- Press **C** to start circle sketching
- Click to set center point, move mouse, click to set radius
- Circle preview is shown in real-time

### 7. Customizable Keybindings
- Go to **Settings → Key Bindings...** to customize all keyboard shortcuts
- Change any keybinding to your preference
- Reset to defaults button available
- Settings are automatically saved and loaded

## Default Keybindings

### View Controls
- **G** - Toggle grid
- **Z** - Toggle wireframe mode  
- **X** - Toggle coordinate axes
- **Home** - Reset view
- **1** - Front view
- **Ctrl+1** - Back view
- **3** - Left view
- **Ctrl+3** - Right view
- **7** - Top view
- **Ctrl+7** - Bottom view
- **9** - Isometric view

### Selection and Editing
- **Ctrl+A** - Select all objects
- **Ctrl+Shift+A** - Deselect all objects
- **Delete** - Delete selected objects

### Tools and Sketching
- **P** - Place shape mode
- **L** - Line sketching
- **R** - Rectangle sketching  
- **C** - Circle sketching
- **Escape** - Cancel current action

### Camera Movement
- **W/A/S/D** - Move camera (forward/left/backward/right)
- **Q/E** - Move camera up/down
- **Middle mouse** - Pan camera
- **Left mouse + drag** - Rotate camera
- **Mouse wheel** - Zoom in/out

## Tips for FreeCAD-like Experience

1. **Use the sketching tools** - The new line, rectangle, and circle sketching tools work similarly to FreeCAD's sketcher
2. **Customize keybindings** - Set up keybindings that match your FreeCAD workflow
3. **Grid snapping** - Enable grid snapping for precise placement (coming soon)
4. **Multiple views** - Use the numbered keys to quickly switch between standard views

## Building and Running

1. Make sure you have Qt6 installed
2. Build the project:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
3. Run the application:
   ```bash
   ./HybridCAD
   ```

The application now provides a much more responsive and feature-rich CAD experience with proper grid toggling, smooth camera movement, and sketching capabilities similar to FreeCAD! 