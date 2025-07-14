#!/bin/bash
# File: scripts/compile_textures
set -euo pipefail

C:\Users\astor_dev\Downloads\texconv.exe -f BC3_UNORM -ft dds -o ./assets/textures/ ./assets/image/test.png
# Microsoft (R) DirectX Texture Converter [DirectXTex] Version 2025.3.25.2
# Copyright (C) Microsoft Corp.

# reading .\assets\image\test.png (956x1680 B8G8R8A8_UNORM 2D) as (956x1680,11 BC3_UNORM 2D a:NonPM)
# writing .\assets\textures\test.dds