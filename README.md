# LODGenerator

Simple batch LOD generation tool for **Unreal Engine 5**.

LODGenerator allows you to generate LODs for multiple Static Meshes at once using Unreal Engine **LOD Groups** or custom reduction settings.

## Notes

LODGenerator is **editor-only** type plugin.

## Features

- Batch LOD generation for multiple Static Meshes
- Supports selected Static Mesh assets and folders
- Uses Unreal Engine LOD Groups
- Custom LOD reduction map
- Configure:
  - Percent Triangles
  - Screen Size
- Add or remove LOD levels directly from the tool
- Automatically saves processed meshes
- Progress window with cancellation support
- Integrated directly into the Unreal Editor toolbar

## Usage

1. Select one or more Static Meshes or folders in the Content Browser.
2. Open **LODGenerator** from the editor toolbar.
3. Select an LOD Group.
4. Configure the LOD Reduction Map if needed.
5. Click **Generate LODs From Selection**.

The plugin will process all Static Meshes found in the current selection.

## Installation

Download release and copy the plugin into:

```text
YourProject/Plugins/LODGenerator
```

## Requirements

- Unreal Engine 5 (release is compiled only for UE5.6)
- PythonScriptPlugin
