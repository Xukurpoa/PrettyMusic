# Pretty Music  

## Project Plan

### Step 1 (Rainmeter inspired)

Create WASAPI capture client in loopback mode and read byte stream

Take byte stream from windows audio output device and run FFT calculations based on kiss_fftr library and Rainmeter audio client implementation.  
Process FFT calculated values in bands (n=10) and output values to log file  
Using easylogging++ as logging solution
  

### Step 1.5 (Generate Visualization)

Using fft loudness array previously calculated create gui based frequency renderer

Using <fill in later> gui builder to display frequencies

Register program as service/run in background

### Step 2 (Light Strip)

Synchronize colored lights to audio output derived in previous step

Use Arduino to handle changing light colors

### Step 3 (Remote)

Try to get bluetooth updates for arduino and pc

Create custom bluetooth driver

### Step 4 (Cube)

Cover lights in resin to ensure vibeyness

## TODO

Clean up code and put better logging expressions  
Add in error checks and clean up memory leaks plus clean up program state  
Create visualization and wait for light strips 