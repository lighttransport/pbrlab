# tinybvh Integration in pbrlab

This directory contains the tinybvh raytracing library integrated as an alternative backend to Embree in pbrlab.

## Build Options

### Backend Selection

Choose between tinybvh (default) or Embree backend:

```bash
# Use tinybvh backend (default)
cmake -DPBRLAB_WITH_EMBREE=OFF ..
# or simply:
cmake ..

# Use Embree backend
cmake -DPBRLAB_WITH_EMBREE=ON ..
```

### SIMD Instruction Control (tinybvh only)

When using tinybvh backend, you can control SIMD instruction usage:

```bash
# tinybvh with SSE4.1/NEON SIMD optimizations (default)
cmake -DPBRLAB_TINYBVH_NO_SIMD=OFF ..
# or simply:
cmake ..

# tinybvh with all SIMD disabled (maximum compatibility)
cmake -DPBRLAB_TINYBVH_NO_SIMD=ON ..
```

## Performance vs Compatibility

| Configuration | Performance | Compatibility | Use Case |
|---------------|-------------|---------------|----------|
| tinybvh + SSE4.1 (default) | High | Good | Default choice, modern x86/ARM |
| tinybvh + No SIMD | Lower | Maximum | Older CPUs, embedded systems, debugging |
| Embree + SSE2 | Highest | Good | Maximum performance, modern x86 systems |

## Implementation Details

- **SIMD Support**: When enabled, tinybvh uses SSE4.1 on x86 and NEON on ARM64
- **AVX/AVX512**: Explicitly disabled for better compatibility
- **Fallback**: Pure C++ reference implementation when SIMD is disabled
- **API Compatibility**: Both backends provide identical raytracer interface

## Compile Definitions

The following preprocessor macros control the behavior:

- `PBRLAB_WITH_EMBREE`: Enables Embree backend
- `PBRLAB_TINYBVH_NO_SIMD`: Disables all SIMD in tinybvh when defined
- `TINYBVH_NO_SIMD`: Internal tinybvh macro set automatically

## Files

- `tiny_bvh.h`: Main tinybvh header (modified for pbrlab)
- `tiny_bvh_minimal.cpp`: Example usage
- `LICENSE`: tinybvh license (MIT)
- `README.md`: Original tinybvh documentation

## Notes

- tinybvh does not currently support cubic Bezier curve meshes
- Only triangle mesh geometry is fully supported
- The reference implementation (no SIMD) still provides good performance for most use cases