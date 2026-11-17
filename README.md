 # Real-Time Guitar Audio Processing System

## Project Team

| Jamal Antabeel | Joaquin Plaza Herrera | Dhared Aldrich Drew | Avgusta Gasenko |
|:---:|:---:|:---:|:---:|

---

## Overview

We are developing a guitar key detection system that identifies musical notes and chords played on an electric guitar.

The system uses an analog front-end circuit to condition the guitar signal before converting it into digital samples using an **MCP3208 12-bit ADC**. A **BeagleY-AI** then processes the sampled signal using C/C++ to analyze its frequency content and identify musical chords.

The goal is to develop a system capable of recognizing multiple guitar chords in real time, combining analog electronics, digital signal processing, and embedded software.

## System Architecture

The project is divided into three main stages:

### 1. Analog Signal Conditioning

- AC coupling and DC biasing.
- Signal amplification using an MCP6002 operational amplifier.
- Low-pass filtering to reduce unwanted high-frequency components before sampling.

### 2. Analog-to-Digital Conversion

- MCP3208 12-bit ADC.
- SPI communication between the ADC and BeagleY-AI.
- Guitar signal sampled through ADC channel 0.

### 3. Digital Signal Processing

- C/C++ software running on the BeagleY-AI.
- Analysis of sampled guitar signals.
- Identification of musical notes and chords.

## Current Progress

The current prototype successfully captures guitar input and identifies several chords, including:

- A minor (Am)
- B minor (Bm)
- C major (C)
- G major (G)
- D major (D)
- E major (E)

Further development will focus on improving chord recognition, refining signal processing, and expanding the range of detectable chords.
