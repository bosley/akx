#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXTENSION_NAME="akx-language"
INSTALL_CURSOR=false

detect_vscode_dir() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        VSCODE_DIR="$HOME/.vscode/extensions"
        VSCODE_INSIDERS_DIR="$HOME/.vscode-insiders/extensions"
        CURSOR_DIR="$HOME/.cursor/extensions"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        VSCODE_DIR="$HOME/.vscode/extensions"
        VSCODE_INSIDERS_DIR="$HOME/.vscode-insiders/extensions"
        CURSOR_DIR="$HOME/.cursor/extensions"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
        VSCODE_DIR="$USERPROFILE/.vscode/extensions"
        VSCODE_INSIDERS_DIR="$USERPROFILE/.vscode-insiders/extensions"
        CURSOR_DIR="$USERPROFILE/.cursor/extensions"
    else
        echo "Unsupported OS: $OSTYPE"
        exit 1
    fi
}

install_extension() {
    detect_vscode_dir
    
    local installed=false
    
    if [ -d "$VSCODE_DIR" ]; then
        echo "Installing AKX syntax highlighting to VSCode..."
        TARGET="$VSCODE_DIR/$EXTENSION_NAME"
        
        if [ -L "$TARGET" ] || [ -e "$TARGET" ]; then
            echo "  Removing existing installation..."
            rm -rf "$TARGET"
        fi
        
        ln -s "$SCRIPT_DIR" "$TARGET"
        echo "  ✓ Installed to: $TARGET"
        installed=true
    fi
    
    if [ -d "$VSCODE_INSIDERS_DIR" ]; then
        echo "Installing AKX syntax highlighting to VSCode Insiders..."
        TARGET="$VSCODE_INSIDERS_DIR/$EXTENSION_NAME"
        
        if [ -L "$TARGET" ] || [ -e "$TARGET" ]; then
            echo "  Removing existing installation..."
            rm -rf "$TARGET"
        fi
        
        ln -s "$SCRIPT_DIR" "$TARGET"
        echo "  ✓ Installed to: $TARGET"
        installed=true
    fi
    
    if [ "$INSTALL_CURSOR" = true ]; then
        if [ -d "$CURSOR_DIR" ]; then
            echo "Installing AKX syntax highlighting to Cursor..."
            TARGET="$CURSOR_DIR/$EXTENSION_NAME"
            
            if [ -L "$TARGET" ] || [ -e "$TARGET" ]; then
                echo "  Removing existing installation..."
                rm -rf "$TARGET"
            fi
            
            ln -s "$SCRIPT_DIR" "$TARGET"
            echo "  ✓ Installed to: $TARGET"
            installed=true
        else
            echo "Warning: Cursor extensions directory not found: $CURSOR_DIR"
        fi
    fi
    
    if [ "$installed" = false ]; then
        echo "Error: No editor installation found."
        echo "Expected directories:"
        echo "  - $VSCODE_DIR"
        echo "  - $VSCODE_INSIDERS_DIR"
        if [ "$INSTALL_CURSOR" = true ]; then
            echo "  - $CURSOR_DIR"
        fi
        exit 1
    fi
    
    echo ""
    echo "Installation complete!"
    echo ""
    echo "To activate the extension:"
    echo "  1. Reload your editor (Cmd+Shift+P / Ctrl+Shift+P -> 'Developer: Reload Window')"
    echo "  2. Open any .akx file"
    echo ""
}

uninstall_extension() {
    detect_vscode_dir
    
    local uninstalled=false
    
    if [ -d "$VSCODE_DIR" ]; then
        TARGET="$VSCODE_DIR/$EXTENSION_NAME"
        if [ -L "$TARGET" ] || [ -e "$TARGET" ]; then
            echo "Uninstalling from VSCode..."
            rm -rf "$TARGET"
            echo "  ✓ Removed: $TARGET"
            uninstalled=true
        fi
    fi
    
    if [ -d "$VSCODE_INSIDERS_DIR" ]; then
        TARGET="$VSCODE_INSIDERS_DIR/$EXTENSION_NAME"
        if [ -L "$TARGET" ] || [ -e "$TARGET" ]; then
            echo "Uninstalling from VSCode Insiders..."
            rm -rf "$TARGET"
            echo "  ✓ Removed: $TARGET"
            uninstalled=true
        fi
    fi
    
    if [ "$INSTALL_CURSOR" = true ]; then
        if [ -d "$CURSOR_DIR" ]; then
            TARGET="$CURSOR_DIR/$EXTENSION_NAME"
            if [ -L "$TARGET" ] || [ -e "$TARGET" ]; then
                echo "Uninstalling from Cursor..."
                rm -rf "$TARGET"
                echo "  ✓ Removed: $TARGET"
                uninstalled=true
            fi
        fi
    fi
    
    if [ "$uninstalled" = false ]; then
        echo "AKX syntax highlighting is not installed."
        exit 0
    fi
    
    echo ""
    echo "Uninstallation complete!"
    echo "Reload your editor to complete the removal."
    echo ""
}

show_usage() {
    cat << EOF
AKX Syntax Highlighting Manager

Usage: $0 [--install|--uninstall] [--cursor]

Options:
  --install      Install AKX syntax highlighting to VSCode/VSCode Insiders
  --uninstall    Remove AKX syntax highlighting from VSCode/VSCode Insiders
  --cursor       Also install/uninstall for Cursor editor (use with --install or --uninstall)

Examples:
  $0 --install
  $0 --install --cursor
  $0 --uninstall
  $0 --uninstall --cursor

EOF
}

if [ $# -eq 0 ]; then
    show_usage
    exit 1
fi

ACTION=""

for arg in "$@"; do
    case "$arg" in
        --install)
            ACTION="install"
            ;;
        --uninstall)
            ACTION="uninstall"
            ;;
        --cursor)
            INSTALL_CURSOR=true
            ;;
        *)
            echo "Unknown option: $arg"
            show_usage
            exit 1
            ;;
    esac
done

if [ -z "$ACTION" ]; then
    show_usage
    exit 1
fi

case "$ACTION" in
    install)
        install_extension
        ;;
    uninstall)
        uninstall_extension
        ;;
    *)
        show_usage
        exit 1
        ;;
esac

