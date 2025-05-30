# Documentation Guide for ESP32 Patch Bay

This document explains how to generate and view the documentation for the ESP32 Patch Bay project.

## Prerequisites

To generate documentation, you need to have Doxygen installed:

```bash
# For Ubuntu/Debian
sudo apt-get install doxygen

# For Fedora
sudo dnf install doxygen

# For Arch Linux
sudo pacman -S doxygen

# For macOS
brew install doxygen
```

## Generating Documentation

To generate the documentation, follow these steps:

1. Open a terminal in the project root directory
2. Run the Doxygen command:

```bash
doxygen Doxyfile
```

3. The documentation will be generated in the `docs/doxygen/html` directory

## Viewing Documentation

To view the generated documentation:

1. Open the `docs/doxygen/html/index.html` file in your web browser:

```bash
# Using xdg-open
xdg-open docs/doxygen/html/index.html

# Or using a specific browser
firefox docs/doxygen/html/index.html
```

## Documentation Structure

The generated documentation includes:

- **Main Page**: Overview of the project from the README.md
- **Files**: All source files with their documentation
- **Data Structures**: Structures and typedefs defined in the code
- **Functions**: All documented functions
- **Variables**: Global and static variables

## Updating Documentation

The documentation is automatically generated from:

1. Doxygen comments in the source code
2. Markdown files in the docs directory
3. Project README.md

To update the documentation:
1. Update the relevant Doxygen comments in your source code
2. Modify or add markdown files in the docs directory
3. Regenerate the documentation using the steps above