Hip-Hop / High Performance Hybrid Audio Plugins
-----------------------------------------------

This project provides [DPF](http://github.com/DISTRHO/DPF) compatible classes
and a handy Makefile for creating audio plugins that run their UI in a web view.
Main goal is to facilitate the creation of complex user interfaces leveraging
JavaScript, HTML and related technologies. The web UI runs completely decoupled
from the DSP core. The latter can be implemented in C++ or optionally [AssemblyScript](https://www.assemblyscript.org).

![Screenshot_2021-10-24_18-30-37](https://user-images.githubusercontent.com/930494/138603460-e8407acb-35db-4bcb-b991-7b7cd7e74381.png)

*WebGain example running on Bitwig for Linux*

****

### Features

* Based on DISTRHO Plugin Framework (DPF)
* C++ or AssemblyScript for DSP code
* HTML / CSS / JS for UI development
* VST2 / LV2 plugin formats
* Linux / Mac / Windows
* Just the powerful basics

VST3 support is almost complete, check DPF's GitHub page for further details.

The following language combinations are possible:

DSP|UI |Comments
---|---|---------------------------------------------------------------------------
C++|JS |Web view user interface, see example [webgain](https://github.com/lucianoiam/hiphop/tree/master/examples/webgain).
AS |JS |Web view user interface, see example [jitdrum](https://github.com/lucianoiam/hiphop/tree/master/examples/jitdrum).
AS |C++|DPF Graphics Library (DGL), see example [astone](https://github.com/lucianoiam/hiphop/tree/master/examples/astone).
C++|C++|Do not use this project, just use DPF instead.

For information on the implementation go [here](https://github.com/lucianoiam/hiphop/blob/master/doc/internals.md).

There are no known stopper bugs, however there is a [list](https://github.com/lucianoiam/hiphop/blob/master/doc/bugs.txt) of smaller details that would be nice to
address at some point.

### Example JavaScript UI code

```JavaScript
class ExampleUI extends DISTRHO.UI {

    constructor() {
        super();
    
        // Connect <input type="range" id="gain"> element to a parameter

        document.getElementById('gain').addEventListener('input', (ev) => {
            this.setParameterValue(0, parseFloat(ev.target.value));
        });
    }

    parameterChanged(index, value) {
        // Host informs a parameter change, update input element value

        switch (index) {
            case 0:
                document.getElementById('gain').value = value;
                break;
        }
    }
    
}
```

The complete UI interface is defined [here](https://github.com/lucianoiam/hiphop/blob/master/src/ui/dpf.js).

### Example AssemblyScript DSP code

```TypeScript
export default class ExamplePlugin extends DISTRHO.Plugin implements DISTRHO.PluginInterface {

    private gain: f32

    setParameterValue(index: u32, value: f32): void {
        // Host informs a parameter change, update local value

        switch (index) {
            case 0:
                this.gain = value
        }
    }

    run(inputs: Float32Array[], outputs: Float32Array[], midiEvents: DISTRHO.MidiEvent[]): void {
        // Process a single audio channel, input and output buffers have equal size

        for (let i = 0; i < inputs[0].length; ++i) {
            outputs[0][i] = this.gain * inputs[0][i]
        }
    }

}
```

The complete plugin interface is defined [here](https://github.com/lucianoiam/hiphop/blob/master/src/dsp/dpf.ts).

### Plugin implementations

[Castello Reverb](https://github.com/lucianoiam/castello)

### About DISTRHO Plugin Framework (DPF)

The framework sports an accurate motto: "DPF is designed to make development of
new plugins an easy and enjoyable task". It is a low footprint yet powerful tool
that allows developers to focus on productivity by already solving many non
trivial issues commonly found in the audio plugins domain.

What makes it great?

- Bloat-free: scope is limited to audio plugins
- Makefile based, MinGW compatible
- Extremely low learning curve
- High quality clean C++ codebase
- Great experienced community around

If you find libre software useful please support the developers
