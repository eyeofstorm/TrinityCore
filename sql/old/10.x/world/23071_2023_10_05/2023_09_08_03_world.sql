SET @TEL_ID := 1983;

DELETE FROM `game_tele` WHERE `id` BETWEEN @TEL_ID+0 AND @TEL_ID+14;
INSERT INTO `game_tele` (`id`,`position_x`,`position_y`,`position_z`,`orientation`,`map`,`name`) VALUES
(@TEL_ID+0,6803.99,5474.99,29.5745,5.49795,1064,'IsleOfThunder'),
(@TEL_ID+1,7237.55,5291.68,65.9856,5.49167,1064,'TheThunderForges'),
(@TEL_ID+2,7444.27,5574.92,51.5438,1.54897,1064,'LightningVeinMine'),
(@TEL_ID+3,7088.83,5605.44,29.5338,3.88789,1064,'TheBeastPens'),
(@TEL_ID+4,6700.59,5353.72,30.7285,5.48224,1064,'ConquerorsTerrace'),
(@TEL_ID+5,7004.14,5041.72,23.2763,3.91536,1064,'StormseaLanding'),
(@TEL_ID+6,7087.32,5191.69,66.0389,5.48616,1064,'TheFootOfLeiShen'),
(@TEL_ID+7,7000.75,5278.47,84.4465,5.49009,1064,'BloodiedCrossing'),
(@TEL_ID+8,6803.96,5475.07,29.5742,5.49402,1064,'TheEmperorsGate'),
(@TEL_ID+9,6546.15,5733.32,28.7805,5.50189,1064,'Diremoor'),
(@TEL_ID+10,6147.58,5006.16,36.0185,2.63752,1064,'VioletRise'),
(@TEL_ID+11,5827.76,5517.46,41.0009,5.92207,1064,'IhgalukCrag'),
(@TEL_ID+12,5893.05,6208.06,4.25242,0.0300062,1064,'ZaTual'),
(@TEL_ID+13,6616.48,6201.95,32.4691,3.13548,1064,'CourtOfBones'),
(@TEL_ID+14,7176.19,6300.87,12.9138,5.48382,1064,'DawnseekerPromontory');
