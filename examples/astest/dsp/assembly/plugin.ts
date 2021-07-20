import { DISTRHO_Plugin_Base, DISTRHO_Plugin } from './distrho-plugin'

@inline const PI2: f32 = Mathf.PI * 2

export default class AsTestPlugin extends DISTRHO_Plugin_Base implements DISTRHO_Plugin {

  private phase: f32

  getLabel(): string {
    return "AsTest"
  }

  getMaker(): string {
    return "Luciano Iam"
  }

  getLicense(): string {
    return "ISC"
  }

  activate(): void {
    // Empty implementation
  }

  deactivate(): void {
    // Empty implementation
  }

  run(inputs: Float32Array[], outputs: Float32Array[]): void {
    const output = outputs[0]
    const frequency: f32 = 440.0
    const gain: f32 = 1.0

    let sample: f32
    let phase: f32 = this.phase

    const radiansPerSample = this.freqToRadians(frequency)
    for (let i = 0; i < output.length; ++i) {
      sample = Mathf.sin(phase)
      phase += radiansPerSample
      output[i] = sample * gain
    }

    while (phase > PI2) phase -= PI2

    this.phase = phase
  }

  @inline freqToRadians(frequency: f32): f32 {
    return frequency * PI2 * (1.0 / this.getSampleRate())
  }

}
