.text
movz X0, 2
cmp X0, X11
bne foo
adds X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X11
bne bar
adds X3, X0, 10
HLT 0
