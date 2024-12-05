/* index: chord_len/r,  sect_area/(r*r)  pairs */
static const double arc_area_factor[] = {
	0.0000000000000000, 0.0000000000000000,
	0.0001562479655054, 0.0000001627591451,
	0.0006249674485948, 0.0000013020426438,
	0.0014060852128027, 0.0000043942222699,
	0.0024994792100675, 0.0000104153646608,
	0.0039049785998017, 0.0000203410788099,
	0.0056223637755851, 0.0000351463636288,
	0.0076513663994780, 0.0000558054556064,
	0.0099916694439485, 0.0000832916765859,
	0.0126429072414071, 0.0001185772816857,
	0.0156046655413419, 0.0001626333073862,
	0.0188764815750443, 0.0002164294198073,
	0.0224578441279154, 0.0002809337632004,
	0.0263481936193427, 0.0003571128086775,
	0.0305469221901331, 0.0004459312032020,
	0.0350533737974895, 0.0005483516188651,
	0.0398668443175168, 0.0006653346024694,
	0.0449865816552391, 0.0007978384254463,
	0.0504117858621135, 0.0009468189341273,
	0.0561416092610202, 0.0011132294003955,
	0.0621751565787105, 0.0012980203727385,
	0.0685114850856911, 0.0015021395277273,
	0.0751496047435241, 0.0017265315219436,
	0.0820884783595183, 0.0019721378443791,
	0.0893270217487880, 0.0022398966693302,
	0.0968641039036557, 0.0025307427098096,
	0.1046985471703686, 0.0028456070714995,
	0.1128291274331057, 0.0031854171072686,
	0.1212545743052423, 0.0035510962722743,
	0.1299735713278462, 0.0039435639796757,
	0.1389847561753716, 0.0043637354569762,
	0.1482867208685190, 0.0048125216030219,
	0.1578780119942300, 0.0052908288456747,
	0.1677571309327797, 0.0057995590001857,
	0.1779225340919328, 0.0063396091282876,
	0.1883726331481273, 0.0069118713980310,
	0.1991057952946463, 0.0075172329443849,
	0.2101203434967410, 0.0081565757306237,
	0.2214145567536638, 0.0088307764105223,
	0.2329866703675714, 0.0095407061913804,
	0.2448348762192547, 0.0102872306978985,
	0.2569573230506550, 0.0110712098369258,
	0.2693521167541175, 0.0118934976631011,
	0.2820173206683435, 0.0127549422454090,
	0.2949509558809886, 0.0136563855346704,
	0.3081510015378641, 0.0145986632319899,
	0.3216153951586918, 0.0155826046581787,
	0.3353420329593613, 0.0166090326241752,
	0.3493287701806432, 0.0176787633024823,
	0.3635734214233028, 0.0187926060996424,
	0.3780737609895639, 0.0199513635297689,
	0.3928275232308702, 0.0211558310891553,
	0.4078324029018880, 0.0224067971319802,
	0.4230860555206959, 0.0237050427471290,
	0.4385860977351058, 0.0250513416361500,
	0.4543301076950565, 0.0264464599923653,
	0.4703156254310226, 0.0278911563811544,
	0.4865401532383805, 0.0293861816214303,
	0.5030011560676689, 0.0309322786683251,
	0.5196960619206860, 0.0325301824971053,
	0.5366222622523573, 0.0341806199883328,
	0.5537771123783176, 0.0358843098142920,
	0.5711579318881361, 0.0376419623266981,
	0.5887620050641258, 0.0394542794457064,
	0.6065865813056679, 0.0413219545502385,
	0.6246288755589889, 0.0432456723696436,
	0.6428860687523187, 0.0452261088767106,
	0.6613553082363652, 0.0472639311820488,
	0.6800337082300341, 0.0493597974298535,
	0.6989183502713278, 0.0515143566950718,
	0.7180062836733480, 0.0537282488819863,
	0.7372945259853350, 0.0560021046242314,
	0.7567800634586692, 0.0583365451862581,
	0.7764598515177636, 0.0607321823662622,
	0.7963308152357729, 0.0631896184005927,
	0.8163898498150428, 0.0657094458696532,
	0.8366338210722308, 0.0682922476053128,
	0.8570595659280140, 0.0709385965998394,
	0.8776638929013146, 0.0736490559163709,
	0.8984435826079630, 0.0764241786009379,
	0.9193953882637180, 0.0792645075960514,
	0.9405160361915724, 0.0821705756558689,
	0.9618022263332578, 0.0851429052629529,
	0.9832506327648708, 0.0881820085466338,
	1.0048579042165431, 0.0912883872029911,
	1.0266206645960703, 0.0944625324164640,
	1.0485355135164192, 0.0977049247831051,
	1.0705990268270322, 0.1010160342354880,
	1.0928077571488419, 0.1043963199692818,
	1.1151582344129192, 0.1078462303715021,
	1.1376469664026638, 0.1113662029504519,
	1.1602704392994556, 0.1149566642673613,
	1.1830251182316815, 0.1186180298697389,
	1.2059074478270528, 0.1223507042264426,
	1.2289138527681238, 0.1261550806644828,
	1.2520407383509298, 0.1300315413075654,
	1.2752844910466490, 0.1339804570163861,
	1.2986414790662117, 0.1380021873306837,
	1.3221080529277611, 0.1420970804130632,
	1.3456805460268724, 0.1462654729945959,
	1.3693552752094580, 0.1505076903222061,
	1.3931285413472489, 0.1548240461078513,
	1.4169966299157770, 0.1592148424795061,
	1.4409558115747643, 0.1636803699339550,
	1.4650023427508199, 0.1682209072914025,
	1.4891324662223715, 0.1728367216519089,
	1.5133424117067185, 0.1775280683536554,
	1.5376283964491337, 0.1822951909330489,
	1.5619866258139115, 0.1871383210866694,
	1.5864132938772699, 0.1920576786350681,
	1.6109045840220197, 0.1970534714884210,
	1.6354566695339041, 0.2021258956140430,
	1.6600657141995123, 0.2072751350057687,
	1.6847278729056823, 0.2125013616552036,
	1.7094392922402915, 0.2178047355248510,
	1.7341961110943436, 0.2231854045231178,
	1.7589944612652608, 0.2286435044812044,
	1.7838304680612804, 0.2341791591318811,
	1.8087002509068701, 0.2397924800901555,
	1.8335999239490575, 0.2454835668358326,
	1.8585255966645875, 0.2512525066979712,
	1.8834733744678098, 0.2570993748412392,
	1.9084393593191984, 0.2630242342541690,
	1.9334196503344145, 0.2690271357393157,
	1.9584103443938081, 0.2751081179053198,
	1.9834075367522763, 0.2812672071608738,
	2.0084073216493694, 0.2875044177105966,
	2.0334057929195595, 0.2938197515528138,
	2.0583990446025697, 0.3002131984792455,
	2.0833831715536739, 0.3066847360766013,
	2.1083542700538653, 0.3132343297300822,
	2.1333084384198062, 0.3198619326287895,
	2.1582417776134601, 0.3265674857730385,
	2.1831503918513042, 0.3333509179835771,
	2.2080303892130395, 0.3402121459127079,
	2.2328778822496966, 0.3471510740573103,
	2.2576889885910414, 0.3541675947737634,
	2.2824598315521856, 0.3612615882947648,
	2.3071865407393126, 0.3684329227480438,
	2.3318652526544188, 0.3756814541769667,
	2.3564921112989761, 0.3830070265630290,
	2.3810632687764239, 0.3904094718502339,
	2.4055748858933961, 0.3978886099713498,
	2.4300231327595880, 0.4054442488760459,
	2.4544041893861652, 0.4130761845608997,
	2.4787142462826344, 0.4207842011012720,
	2.5029495050520638, 0.4285680706850447,
	2.5271061789845763, 0.4364275536482158,
	2.5511804936490172, 0.4443623985123471,
	2.5751686874826993, 0.4523723420238562,
	2.5990670123791397, 0.4604571091951500,
	2.6228717342736942, 0.4686164133475910,
	2.6465791337269984, 0.4768499561562896,
	2.6701855065061189, 0.4851574276967176,
	2.6936871641633360, 0.4935385064931327,
	2.7170804346124569, 0.5019928595688086,
	2.7403616627025649, 0.5105201424980620,
	2.7635272107891375, 0.5191199994600683,
	2.7865734593024163, 0.5277920632944579,
	2.8094968073129589, 0.5365359555586845,
	2.8322936730942754, 0.5453512865871555,
	2.8549604946824685, 0.5542376555521169,
	2.8774937304327795, 0.5631946505262819,
	2.8998898595729665, 0.5722218485471927,
	2.9221453827534178, 0.5813188156833089,
	2.9442568225939123, 0.5904851071018076,
	2.9662207242269627, 0.5997202671380879,
	2.9880336558376230, 0.6090238293669674,
	3.0096922091997085, 0.6183953166755602,
	3.0311930002083192, 0.6278342413378251,
	3.0525326694086035, 0.6373401050907712,
	3.0737078825206661, 0.6469123992123116,
	3.0947153309605366, 0.6565506046007487,
	3.1155517323571376, 0.6662541918558837,
	3.1362138310651417, 0.6760226213617349,
	3.1566983986736621, 0.6858553433708494,
	3.1770022345106872, 0.6957517980902029,
	3.1971221661431746, 0.7057114157686640,
	3.2170550498727408, 0.7157336167860178,
	3.2367977712268554, 0.7258178117435294,
	3.2563472454454754, 0.7359634015560379,
	3.2757004179630260, 0.7461697775455604,
	3.2948542648856747, 0.7564363215363976,
	3.3138057934638105, 0.7667624059517230,
	3.3325520425596471, 0.7771473939116390,
	3.3510900831099035, 0.7875906393326898,
	3.3694170185834604, 0.7980914870288094,
	3.3875299854339347, 0.8086492728136935,
	3.4054261535471073, 0.8192633236045774,
	3.4231027266831191, 0.8299329575274039,
	3.4405569429133842, 0.8406574840233640,
	3.4577860750521228, 0.8514362039567964,
	3.4747874310824924, 0.8622684097244249,
	3.4915583545772018, 0.8731533853659208,
	3.5080962251135759, 0.8840904066757700,
	3.5243984586829895, 0.8950787413164286,
	3.5404625080946164, 0.9061176489327490,
	3.5562858633734145, 0.9172063812676594,
	3.5718660521523162, 0.9283441822790766,
	3.5872006400585219, 0.9395302882580367,
	3.6022872310938694, 0.9507639279480233,
	3.6171234680092041, 0.9620443226654738,
	3.6317070326726895, 0.9733706864214480,
	3.6460356464320216, 0.9847422260444367,
	3.6601070704704473, 0.9961581413042939,
	3.6739191061565917, 1.0076176250372697,
	3.6874695953879786, 1.0191198632721301,
	3.7007564209282333, 1.0306640353573364,
	3.7137775067378982, 1.0422493140892708,
	3.7265308182988055, 1.0538748658414865,
	3.7390143629319725, 1.0655398506949587,
	3.7512261901089454, 1.0772434225693206,
	3.7631643917565749, 1.0889847293550623,
	3.7748271025551410, 1.1007629130466705,
	3.7862125002298086, 1.1125771098766903,
	3.7973188058353564, 1.1244264504506876,
	3.8081442840341255, 1.1363100598830895,
	3.8186872433671759, 1.1482270579338834,
	3.8289460365185595, 1.1601765591461537,
	3.8389190605727164, 1.1721576729844314,
	3.8486047572649311, 1.1841695039738394,
	3.8580016132247992, 1.1962111518400076,
	3.8671081602126947, 1.2082817116497404,
	3.8759229753491784, 1.2203802739524092,
	3.8844446813373210, 1.2325059249220534,
	3.8926719466778952, 1.2446577465001627,
	3.9006034858774314, 1.2568348165391225,
	3.9082380596490687, 1.2690362089462983,
	3.9155744751061849, 1.2812609938287345,
	3.9226115859487916, 1.2935082376384484,
	3.9293482926426364, 1.3057770033182965,
	3.9357835425910048, 1.3180663504483865,
	3.9419163302991849, 1.3303753353930161,
	3.9477456975315737, 1.3427030114481149,
	3.9532707334614012, 1.3550484289891642,
	3.9584905748130441, 1.3674106356195743,
	3.9634044059969109, 1.3797886763194944,
	3.9680114592368771, 1.3921815935950328,
	3.9723110146902454, 1.4045884276278626,
	3.9763024005602245, 1.4170082164251925,
	3.9799849932008939, 1.4294399959700752,
	3.9833582172146436, 1.4418828003720336,
	3.9864215455420848, 1.4543356620179793,
	3.9891744995443990, 1.4667976117233994,
	3.9916166490781246, 1.4792676788837911,
	3.9937476125623639, 1.4917448916263150,
	3.9955670570384108, 1.5042282769616493,
	3.9970746982217649, 1.5167168609360171,
	3.9982703005465603, 1.5292096687833650,
	3.9991536772023650, 1.5417057250776676,
	3.9997246901633732, 1.5542040538853368,
	3.9999832502099730, 1.5667036789177080,
	4.0000000000000000, 1.5707963272051035
};
