//
// voltage divider resistance: 620
// resistance at 25 deg. celsius: 100000
// thermistor B value: 4267
//
const short temptable_2007[][2] PROGMEM = {
    { OV(90), 320 }, // Projected value just to trigger error on short circuit
    { OV(120), 300 },
    { OV(155), 280 },
    { OV(200), 260 },
    { OV(259), 240 },
    { OV(335), 220 },
    { OV(428), 200 },
    { OV(535), 180 },
    { OV(649), 160 },
    { OV(758), 140 },
    { OV(851), 120 },
    { OV(921), 100 },
    { OV(958), 85 },
    { OV(967), 80 },
    { OV(996), 60 },
    { OV(1004), 50 },
    { OV(1011), 40 },
    { OV(1017), 25 },
    { OV(1020), 10 },
    { OV(1021), 0 },
    //{ OV(1022), -10 },
    //{ OV(1023), -30 },
    //{ OV(1023), -50 },
};
