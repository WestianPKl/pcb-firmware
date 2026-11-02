VDDA = 3.3

# Divider 1 12V:
r1 = 180000
r2 = 27000

# Divider 2 12V:
r3 = 180000
r4 = 27000

# Divider 3 5V:
r5 = 68000
r6 = 27000

# Divider 4 3.3V:
r7 = 33000
r8 = 27000

ADC1 = 2019
ADC2 = 1919
ADC3 = 1742
ADC4 = 1865

vadc1 = ADC1 * VDDA / 4095.0
vadc2 = ADC2 * VDDA / 4095.0
vadc3 = ADC3 * VDDA / 4095.0
vadc4 = ADC4 * VDDA / 4095.0

vin1 = vadc1 * (r1 + r2) / r2
vin2 = vadc2 * (r3 + r4) / r4
vin3 = vadc3 * (r5 + r6) / r6
vin4 = vadc4 * (r7 + r8) / r8

print(vin1)
print(vin2)
print(vin3)
print(vin4)
