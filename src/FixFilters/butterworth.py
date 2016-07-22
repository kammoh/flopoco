import numpy as np
from scipy import signal
import matplotlib.pyplot as plt

b, a = signal.butter(3, 0.1, 'low', analog=False)
w, h = signal.freqs(b, a)
plt.semilogx(w, 20 * np.log10(abs(h)))
plt.title('Butterworth filter frequency response')
plt.xlabel('Frequency [radians / second]')
plt.ylabel('Amplitude [dB]')
plt.margins(0, 0.1)
plt.grid(which='both', axis='both')
plt.axvline(100, color='green') # cutoff frequency
#plt.show()


coeffa = ":".join(str(m) for m in a);
print coeffa
coeffa = ":".join(str(m) for m in a[1:]);
print coeffa

coeffa = ":".join(float.hex(m) for m in a[1:]);
coeffb = ":".join(float.hex(m) for m in b);

flopocostring =  './flopoco generateFigures=1 FixIIR coeffb="' + coeffb + '" coeffa="' + coeffa + '" lsbIn=-12 lsbOut=-12 TestBench n=100'

print flopocostring

