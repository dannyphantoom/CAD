# HybridCAD - Advanced CAD & Mesh Editor

HybridCAD is a powerful, cross-platform Computer-Aided Design (CAD) application that combines the precision of traditional CAD software like FreeCAD with the advanced mesh editing capabilities found in Blender. Built with modern C++, Qt6, and OpenCASCADE, it provides a professional-grade solution for both engineering design and creative modeling.

## Features

### CAD Capabilities
- **Precision Geometry**: Real-world accurate geometric modeling using OpenCASCADE kernel
- **Parametric Design**: Fully parametric objects with history-based modeling
- **Boolean Operations**: Union, difference, and intersection operations
- **Primitive Creation**: Box, cylinder, sphere, cone, and custom shapes
- **Sketching Tools**: 2D constraint-based sketching with extrusion and revolution
- **Assembly Management**: Multi-part assemblies with constraints and collision detection

### Mesh Editing (Blender-style)
- **Vertex/Edge/Face Selection**: Multiple selection modes for precise editing
- **Mesh Operations**: Extrude, inset, subdivide, smooth, and decimate
- **Advanced Tools**: Knife tool, loop cuts, bridge edge loops
- **Subdivision Surfaces**: Catmull-Clark and Loop subdivision algorithms
- **Mesh Boolean**: Boolean operations on mesh objects
- **Import/Export**: Support for OBJ, STL, and other mesh formats

### User Interface
- **Modern UI**: Dark theme with dockable panels and customizable toolbars
- **3D Viewport**: Real-time OpenGL rendering with multiple view modes
- **Property Panel**: Context-sensitive property editing
- **Scene Tree**: Hierarchical object management with drag-and-drop
- **Tool Manager**: Comprehensive tool selection with snap modes

### File Formats
- **Native Format**: .cad files for complete project storage
- **CAD Import/Export**: STEP, IGES for industry standard interchange
- **Mesh Formats**: STL, OBJ for 3D printing and exchange
- **Assembly Support**: Multi-file project management

## Requirements

### System Requirements
- **Operating System**: Windows 10+, macOS 10.15+, or Linux (Ubuntu 20.04+)
- **Memory**: 4GB RAM minimum, 8GB recommended
- **Graphics**: OpenGL 3.3 compatible graphics card
- **Storage**: 500MB for installation

### Dependencies
- **Qt6**: Core, Widgets, OpenGL, OpenGLWidgets
- **OpenGL**: Version 3.3 or higher
- **OpenCASCADE**: 7.6+ (optional, for advanced CAD features)
- **CMake**: 3.16+ for building

## Building from Source

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake qt6-base-dev qt6-tools-dev qt6-opengl-dev libgl1-mesa-dev

# macOS (with Homebrew)
brew install cmake qt@6

# Windows (with vcpkg)
vcpkg install qt6-base qt6-tools qt6-opengl
```

### Optional: OpenCASCADE Installation
```bash
# Ubuntu/Debian
sudo apt-get install libopencascade-dev

# macOS (with Homebrew)
brew install opencascade

# Windows
# Download from https://www.opencascade.com/
```

### Build Steps
```bash
# Clone the repository
git clone https://github.com/your-username/HybridCAD.git
cd HybridCAD

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build . --config Release

# Install (optional)
cmake --install .
```

### Running
```bash
# From build directory
./HybridCAD

# Or if installed system-wide
HybridCAD
```

## Usage

### Getting Started
1. **Create Primitives**: Use the Create menu or toolbar to add basic shapes
2. **Edit Properties**: Select objects and modify parameters in the Property Panel
3. **Boolean Operations**: Combine objects using union, difference, or intersection
4. **Mesh Editing**: Enter mesh edit mode (Tab) for detailed mesh manipulation
5. **Assembly**: Create assemblies by adding multiple parts with constraints

### Keyboard Shortcuts
- **S**: Select tool
- **M**: Move tool
- **R**: Rotate tool
- **C**: Scale tool
- **E**: Extrude tool
- **Tab**: Toggle mesh edit mode
- **G**: Toggle grid
- **Z**: Toggle wireframe
- **1-7**: Standard views (Front, Right, Top, etc.)
- **9**: Isometric view
- **Home**: Reset view

### Mouse Controls
- **Left Click**: Select objects
- **Left Drag**: Rotate view
- **Middle Drag**: Pan view
- **Scroll Wheel**: Zoom in/out
- **Right Click**: Context menu

## Architecture

### Core Components
- **CADViewer**: OpenGL-based 3D viewport with camera controls
- **GeometryManager**: Handles CAD geometry creation and boolean operations
- **MeshManager**: Manages mesh editing operations and algorithms
- **PartManager**: Assembly and document management
- **PropertyPanel**: Dynamic property editing interface
- **TreeView**: Hierarchical scene object management

### Design Patterns
- **Observer Pattern**: For UI updates and object notifications
- **Command Pattern**: For undo/redo functionality
- **Factory Pattern**: For object creation
- **Strategy Pattern**: For different tool implementations

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup
1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Submit a pull request

### Coding Standards
- Follow modern C++17 standards
- Use Qt naming conventions for Qt-related code
- Document public APIs with Doxygen comments
- Include unit tests for new features

## License

HybridCAD is released under the MIT License. See [LICENSE](LICENSE) for details.

## Acknowledgments

- **OpenCASCADE**: For the robust CAD geometry kernel
- **Qt**: For the excellent cross-platform GUI framework
- **FreeCAD**: For inspiration and CAD design patterns
- **Blender**: For mesh editing concepts and UI inspiration

## Support

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/your-username/HybridCAD/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-username/HybridCAD/discussions)
- **Wiki**: [GitHub Wiki](https://github.com/your-username/HybridCAD/wiki)

## Roadmap

### Version 1.1
- [ ] Advanced sketching constraints
- [ ] More primitive types
- [ ] Improved mesh boolean operations
- [ ] Plugin system

### Version 1.2
- [ ] Scripting support (Python)
- [ ] Advanced assembly constraints
- [ ] Simulation capabilities
- [ ] Cloud synchronization

### Version 2.0
- [ ] Parametric mesh editing
- [ ] Advanced rendering (PBR)
- [ ] VR/AR support
- [ ] Collaborative editing 