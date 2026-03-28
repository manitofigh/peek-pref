# Peek Pref
A peek into your processor's cache line prefetcher

You can just run `make` at the root to build the program.

At least for Intel SKX-SP processors, the MSR used for enabling/disabling the prefetcher is `0x1a4`. 
You can set the MSR to `0x0` to enable, or `0xf` to disable the prefetcher.

You can use the `rdmsr` and `wrmsr` utilities to see the value or write to the MSR. Make sure to load its 
module using `sudo modprobe msr`. Make sure you have installed the `msr-tools` package first.

The video where I make the tool: https://youtu.be/jEEkGgUWWKI
