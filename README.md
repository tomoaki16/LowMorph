# LowMorph

Experimental VST3 that uses a guitar performance as control/excitation information and resynthesizes a bass-register string instead of simply pitch-shifting the guitar waveform.

## v0.3 prototype
- mono guitar input / mono bass output
- guitar pitch tracking mapped one octave down
- regenerated harmonic string model
- frequency-dependent decay
- pluck-position spectral shaping
- Body / Attack / String / Tone / Mix / Pluck / Pickup / Damping parameters
- no samples, IRs, pretrained models, or third-party recordings

The Windows GitHub Actions workflow builds against Steinberg's official VST3 SDK and uploads the generated VST3 bundle as an artifact.

This is an engineering prototype, not yet a release-quality bass emulation.
