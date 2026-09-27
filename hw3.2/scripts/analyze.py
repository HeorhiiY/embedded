import numpy as np
import matplotlib.pyplot as plt
import os

csv_path = os.path.join(os.path.dirname(__file__), 'data.csv')

data = np.loadtxt(csv_path, delimiter=',', skiprows=1)
raw = data[:, 1]
calibrated_voltage = data[:, 3]
computed_voltage = data[:, 2]
error = data[:, 4]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

ax1.plot(raw, computed_voltage, 'o-', label='(Naive) linear approximation', markersize=4)
ax1.plot(raw, calibrated_voltage, 'o-', label='Calibrated Voltage', markersize=4)
ax1.set_xlabel('Raw')
ax1.set_ylabel('Voltage, mV')
ax1.set_title('Computed Voltage vs Calibrated Voltage')
ax1.grid(True, alpha=0.3)
ax1.legend()

ax2.plot(calibrated_voltage, error, 'o-', label='Error', color='red', markersize=4)
ax2.set_xlabel('Calibrated Voltage, mV')
ax2.set_ylabel('Error, %')
ax2.set_title('Error vs Calibrated Voltage')
ax2.grid(True, alpha=0.3)
ax2.legend()

plt.tight_layout()
plt.savefig(os.path.join(os.path.dirname(__file__), 'analysis.png'), dpi=150)
plt.show()
