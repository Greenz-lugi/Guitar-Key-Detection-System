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

Further development will focus on improving chord recognition, refining signal processing, and expanding the range of detectable chords. Mainly on the software end, having it not lag or be as sensitive to certain signals, where, in some cases, it starts repeating previous chords or becomes sensitive to the signals that came in rather than what is coming in. 

Aside from the software end, we currently have two designs for the hardware, one developed by Jamal and another by Dhared. The more advanced one is coming along as a mix between the two designs to ultimately have the analog aspect of the system be complete. 

Our objective is to make this something professional, manufacturable, and scalable, since in a professional working environment that is the goal of a company. 

So we're planning on potentially looking into PCB design, a 3D model to encase the electronics, and having some Javascript/HTML/CSS into the mix to make this into a website. Goals for us to strive for and implement within our design. 

What to do with the knowledge of the keys played depends on the user; we were thinking an application we can first try and implement is having them act as key binds in video games. Stuff like A minor (Am) being jump, G major (G) being shoot a fireball, and C major (C) being whatever the user would like.

## Progress Currently:



