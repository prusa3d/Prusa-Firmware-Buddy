// voltage divider resistance: 33000
// resistance at 25 deg. celsius: 100000
// thermistor B value: 4267
//
const short temptable_2008[][2] PROGMEM = {
{ OV(3), 300 },
{ OV(3), 280 },
{ OV(5), 260 },
{ OV(6), 240 },
{ OV(9), 220 },
{ OV(14), 200 },
{ OV(21), 180 },
{ OV(32), 160 },
{ OV(52), 140 },
{ OV(87), 120 },
{ OV(148), 100 },
{ OV(221), 85 },
{ OV(252), 80 },
{ OV(415), 60 },
{ OV(515), 50 },
{ OV(621), 40 },
{ OV(769), 25 },
{ OV(883), 10 },
{ OV(936), 0 },
//{ OV(972), -10 },
//{ OV(1008), -30 },
//{ OV(1019), -50 },
};
