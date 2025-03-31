.text
movz X0, 4
cmp X0, X11
ble foo
adds X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X11
ble bar
adds X3, X0, 10
HLT 0