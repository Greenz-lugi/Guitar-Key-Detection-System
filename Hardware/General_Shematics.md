# Hardware Design and Development

## Overview

This directory contains the hardware documentation, circuit schematics, and design decisions associated with the Guitar Key Detection System.

The hardware development focuses on receiving an analogue guitar signal, conditioning it for reliable processing, and preparing it for analogue-to-digital conversion.

The hardware has been developed through researching existing circuit configurations, understanding their individual functions, implementing them, and evaluating their behaviour through simulation and physical testing.

## Hardware Components

The hardware documentation is currently divided into three circuit designs:

### 1. Signal Conditioning Circuit

**Documentation:** `Signal_Conditioning_Circuit.md`

This circuit was developed separately by a project team member. It uses an OP777 operational amplifier as part of a signal-conditioning and buffering stage.

The documentation describes the amplifier's role, the surrounding components, and the intended behaviour of the circuit.

### 2. LED Signal Detection Circuit

**Documentation:** `LED_Signal_Detection_Circuit.md`

This was an earlier working prototype developed to understand and demonstrate guitar signal detection.

The circuit introduced several important analogue electronics concepts, including:

- AC coupling and DC biasing.
- Amplification of an AC signal while maintaining its DC operating point.
- Non-inverting amplifier configurations.
- Schmitt triggers and hysteresis.
- Threshold detection and LED output behaviour.

This prototype provided a practical foundation for understanding how an incoming guitar signal could be conditioned and detected.

### 3. Guitar ADC Input Circuit

**Documentation:** `Guitar_ADC_Input.md`

This is the current analogue input design.

Unlike the earlier LED detector, this circuit is intended to preserve the analogue waveform for digital signal processing.

The design includes:

- AC coupling and DC biasing.
- Non-inverting amplification.
- Low-pass filtering.
- Signal conditioning for the MCP3208 ADC.

The objective is to provide the BeagleY-AI with a suitable digital representation of the guitar signal for frequency analysis and musical note detection.

## Design and Learning Approach

The hardware development follows a process of understanding, implementation, and verification.

Rather than treating circuits as isolated collections of components, the development process focuses on understanding the function of each circuit block.

For each stage, the following questions guide the design:

1. **What is the desired outcome?** Identify what the circuit needs to accomplish.
2. **What circuit configuration can achieve it?** Research suitable topologies and existing implementations.
3. **How does the configuration work?** Understand its inputs, outputs, operating conditions, and limitations.
4. **How should it be implemented?** Select component values and calculate the expected behaviour.
5. **Does the implementation behave as expected?** Simulate, construct, measure, and refine the circuit.

An important part of this process has been recognizing that circuit knowledge can be built from reusable functional blocks. Understanding the purpose and limitations of a particular configuration makes it easier to adapt that configuration to new engineering problems.

## Development Progression

The hardware development has progressed from basic signal detection toward analogue signal acquisition.

The initial LED prototype established an understanding of signal biasing, amplification, and threshold detection. The current design builds on that knowledge by focusing on preserving the guitar waveform for sampling and digital analysis.

These designs document both the practical implementation and the engineering reasoning behind the decisions made throughout development.
