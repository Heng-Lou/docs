# DOCA Working Samples

## Successfully Built Sample

### Flow Control Pipe Example

**Location**: `/home/heng/workdir/doca/flow_control_pipe/`
**Status**: ✅ Built successfully  
**Binary**: `flow_control_pipe/build/doca_flow_control_pipe`

This is an official NVIDIA DOCA sample demonstrating control pipe functionality for packet forwarding.

#### Building

```bash
cd /home/heng/workdir/doca/flow_control_pipe
./build.sh
```

#### Running

```bash
# Requires BlueField DPU hardware or supported NIC
sudo ./build/doca_flow_control_pipe -a <PCI_ADDRESS> -a <PCI_ADDRESS_2> ...
```

#### What This Sample Does

The flow control pipe sample demonstrates:
- Creating DOCA Flow control pipes
- Configuring VXLAN tunnel matching and decapsulation
- Forwarding packets between ports
- Using DOCA Flow API for packet processing

## Available Samples on Your System

All DOCA samples are located in: `/opt/mellanox/doca/samples/`

### DOCA Flow Samples
```bash
ls /opt/mellanox/doca/samples/doca_flow/
```

Examples include:
- `flow_control_pipe` - Control pipe for packet forwarding (✅ copied & built)
- `flow_lpm` - Longest Prefix Match routing
- `flow_entropy` - Entropy-based flow distribution
- `flow_switch_rss` - Receive Side Scaling
- `flow_acl` - Access Control List
- `flow_aging` - Flow aging and timeout
- `flow_ct_*` - Connection tracking samples
- And many more...

### Other DOCA Component Samples

```bash
ls /opt/mellanox/doca/samples/
```

Available samples for:
- **doca_aes_gcm** - AES-GCM encryption/decryption
- **doca_compress** - Hardware compression
- **doca_dma** - DMA operations
- **doca_rdma** - RDMA communication
- **doca_gpunetio** - GPU-accelerated networking
- **doca_sha** - SHA hashing
- **doca_telemetry** - Telemetry and monitoring
- And more...

## How to Use Other Samples

### Method 1: Copy and Build Standalone

```bash
# Copy a sample to your workspace
cd /home/heng/workdir/doca
cp -r /opt/mellanox/doca/samples/doca_flow/<sample_name> .
cd <sample_name>

# Copy common dependencies (already done for flow_control_pipe)
# You may need to adjust meson.build as we did for flow_control_pipe

# Build
./build.sh  # if you create the build script
# or
meson setup build && ninja -C build
```

### Method 2: Build from Original Location

```bash
cd /opt/mellanox/doca/samples/doca_flow/<sample_name>
meson setup build
ninja -C build
```

## Environment Setup

Always source the environment before building:

```bash
cd /home/heng/workdir/doca
source setup_environment.sh
```

This sets up:
- PKG_CONFIG_PATH
- LD_LIBRARY_PATH  
- DOCA_HOME
- PATH for tools

## Hardware Requirements

Most DOCA samples require:
- **BlueField-2 or BlueField-3 DPU**, OR
- **ConnectX-6 Dx or newer** NVIDIA network adapter

Some samples work in emulation mode for testing/development without hardware.

## Running Without Hardware

For development/testing without hardware, you can:
1. Study the sample code to understand DOCA APIs
2. Build samples to verify compilation
3. Use DOCA documentation alongside the code
4. Deploy to actual hardware when ready

Runtime will fail without proper hardware, but you can still learn from the source.

## Documentation

- Sample documentation: Check README or comments in each sample
- DOCA API docs: https://docs.nvidia.com/doca/sdk/
- Local samples: `/opt/mellanox/doca/samples/`

## Next Steps

1. **Explore other flow samples**: Different flow matching and actions
2. **Try DMA samples**: For data movement operations
3. **Check compression samples**: Hardware-accelerated compression
4. **GPU samples**: If you have GPU + DPU setup

## Notes

- Samples are version-matched to your DOCA 3.1.0105 installation
- Source code is MIT/BSD licensed - feel free to use as templates
- Official samples are the best reference for DOCA API usage
- When in doubt, check `/opt/mellanox/doca/samples/` for working examples

---

**Summary**: You now have a working DOCA development environment with a successfully built sample!
