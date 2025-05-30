#!/bin/bash
# Script to generate Doxygen documentation for ESP32 Patch Bay

# Create docs/images directory if it doesn't exist
mkdir -p docs/images

# Run Doxygen
echo "Generating documentation..."
doxygen Doxyfile

# Check if documentation was generated successfully
if [ -d "docs/doxygen/html" ]; then
  echo "Documentation generated successfully!"
  echo "To view documentation, open docs/doxygen/html/index.html in your browser."
  
  # Prompt to open documentation
  read -p "Open documentation now? (y/n): " open_docs
  if [[ $open_docs == "y" || $open_docs == "Y" ]]; then
    if command -v xdg-open &> /dev/null; then
      xdg-open docs/doxygen/html/index.html
    elif command -v open &> /dev/null; then
      open docs/doxygen/html/index.html
    else
      echo "Could not automatically open browser. Please open docs/doxygen/html/index.html manually."
    fi
  fi
else
  echo "Error: Documentation generation failed."
  echo "Please check that Doxygen is installed and the Doxyfile is configured correctly."
fi