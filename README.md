## sips: A simple IPS patch generator

The only functionality of `sips` is generating **correct** IPS32 patches.
No fancy RLE stuff or support for the legacy IPS format.

This is good enough for the intended use case for IPS32 patches in 2019,
which is to make [Switch exefs patches](https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/modules/loader.md#nso-patching).

Apart from [ipswitch](https://github.com/3096/ipswitch), most other tools either are Windows-only,
or generate broken patches, or do not support IPS32.

### Building and using sips
Build with `make`, then `./sips ORIGINAL_FILE PATCHED_FILE PATCH_FILE` to generate a patch file.
