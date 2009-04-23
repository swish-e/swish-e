/*

$Id$


    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.

** Mon May  9 14:51:21 CDT 2005
** added GPL (nothing previously)
    
*/

#include <stdlib.h>
#include "swish.h"
#include "db.h"
#include "rank.h"

/* 1000 precomputed 10000 * log(i) */
static SWINT_T swish_log[] = {\
 0, 0, 6931, 10986, 13863, 16094, 17918, 19459, 20794, 21972,\
 23026, 23979, 24849, 25649, 26391, 27081, 27726, 28332, 28904, 29444,\
 29957, 30445, 30910, 31355, 31781, 32189, 32581, 32958, 33322, 33673,\
 34012, 34340, 34657, 34965, 35264, 35553, 35835, 36109, 36376, 36636,\
 36889, 37136, 37377, 37612, 37842, 38067, 38286, 38501, 38712, 38918,\
 39120, 39318, 39512, 39703, 39890, 40073, 40254, 40431, 40604, 40775,\
 40943, 41109, 41271, 41431, 41589, 41744, 41897, 42047, 42195, 42341,\
 42485, 42627, 42767, 42905, 43041, 43175, 43307, 43438, 43567, 43694,\
 43820, 43944, 44067, 44188, 44308, 44427, 44543, 44659, 44773, 44886,\
 44998, 45109, 45218, 45326, 45433, 45539, 45643, 45747, 45850, 45951,\
 46052, 46151, 46250, 46347, 46444, 46540, 46634, 46728, 46821, 46913,\
 47005, 47095, 47185, 47274, 47362, 47449, 47536, 47622, 47707, 47791,\
 47875, 47958, 48040, 48122, 48203, 48283, 48363, 48442, 48520, 48598,\
 48675, 48752, 48828, 48903, 48978, 49053, 49127, 49200, 49273, 49345,\
 49416, 49488, 49558, 49628, 49698, 49767, 49836, 49904, 49972, 50039,\
 50106, 50173, 50239, 50304, 50370, 50434, 50499, 50562, 50626, 50689,\
 50752, 50814, 50876, 50938, 50999, 51059, 51120, 51180, 51240, 51299,\
 51358, 51417, 51475, 51533, 51591, 51648, 51705, 51761, 51818, 51874,\
 51930, 51985, 52040, 52095, 52149, 52204, 52257, 52311, 52364, 52417,\
 52470, 52523, 52575, 52627, 52679, 52730, 52781, 52832, 52883, 52933,\
 52983, 53033, 53083, 53132, 53181, 53230, 53279, 53327, 53375, 53423,\
 53471, 53519, 53566, 53613, 53660, 53706, 53753, 53799, 53845, 53891,\
 53936, 53982, 54027, 54072, 54116, 54161, 54205, 54250, 54293, 54337,\
 54381, 54424, 54467, 54510, 54553, 54596, 54638, 54681, 54723, 54765,\
 54806, 54848, 54889, 54931, 54972, 55013, 55053, 55094, 55134, 55175,\
 55215, 55255, 55294, 55334, 55373, 55413, 55452, 55491, 55530, 55568,\
 55607, 55645, 55683, 55722, 55759, 55797, 55835, 55872, 55910, 55947,\
 55984, 56021, 56058, 56095, 56131, 56168, 56204, 56240, 56276, 56312,\
 56348, 56384, 56419, 56454, 56490, 56525, 56560, 56595, 56630, 56664,\
 56699, 56733, 56768, 56802, 56836, 56870, 56904, 56937, 56971, 57004,\
 57038, 57071, 57104, 57137, 57170, 57203, 57236, 57268, 57301, 57333,\
 57366, 57398, 57430, 57462, 57494, 57526, 57557, 57589, 57621, 57652,\
 57683, 57714, 57746, 57777, 57807, 57838, 57869, 57900, 57930, 57961,\
 57991, 58021, 58051, 58081, 58111, 58141, 58171, 58201, 58230, 58260,\
 58289, 58319, 58348, 58377, 58406, 58435, 58464, 58493, 58522, 58551,\
 58579, 58608, 58636, 58665, 58693, 58721, 58749, 58777, 58805, 58833,\
 58861, 58889, 58916, 58944, 58972, 58999, 59026, 59054, 59081, 59108,\
 59135, 59162, 59189, 59216, 59243, 59269, 59296, 59322, 59349, 59375,\
 59402, 59428, 59454, 59480, 59506, 59532, 59558, 59584, 59610, 59636,\
 59661, 59687, 59713, 59738, 59764, 59789, 59814, 59839, 59865, 59890,\
 59915, 59940, 59965, 59989, 60014, 60039, 60064, 60088, 60113, 60137,\
 60162, 60186, 60210, 60234, 60259, 60283, 60307, 60331, 60355, 60379,\
 60403, 60426, 60450, 60474, 60497, 60521, 60544, 60568, 60591, 60615,\
 60638, 60661, 60684, 60707, 60730, 60753, 60776, 60799, 60822, 60845,\
 60868, 60890, 60913, 60936, 60958, 60981, 61003, 61026, 61048, 61070,\
 61092, 61115, 61137, 61159, 61181, 61203, 61225, 61247, 61269, 61291,\
 61312, 61334, 61356, 61377, 61399, 61420, 61442, 61463, 61485, 61506,\
 61527, 61549, 61570, 61591, 61612, 61633, 61654, 61675, 61696, 61717,\
 61738, 61759, 61779, 61800, 61821, 61841, 61862, 61883, 61903, 61924,\
 61944, 61964, 61985, 62005, 62025, 62046, 62066, 62086, 62106, 62126,\
 62146, 62166, 62186, 62206, 62226, 62246, 62265, 62285, 62305, 62324,\
 62344, 62364, 62383, 62403, 62422, 62442, 62461, 62480, 62500, 62519,\
 62538, 62558, 62577, 62596, 62615, 62634, 62653, 62672, 62691, 62710,\
 62729, 62748, 62766, 62785, 62804, 62823, 62841, 62860, 62879, 62897,\
 62916, 62934, 62953, 62971, 62989, 63008, 63026, 63044, 63063, 63081,\
 63099, 63117, 63135, 63154, 63172, 63190, 63208, 63226, 63244, 63261,\
 63279, 63297, 63315, 63333, 63351, 63368, 63386, 63404, 63421, 63439,\
 63456, 63474, 63491, 63509, 63526, 63544, 63561, 63578, 63596, 63613,\
 63630, 63648, 63665, 63682, 63699, 63716, 63733, 63750, 63767, 63784,\
 63801, 63818, 63835, 63852, 63869, 63886, 63902, 63919, 63936, 63953,\
 63969, 63986, 64003, 64019, 64036, 64052, 64069, 64085, 64102, 64118,\
 64135, 64151, 64167, 64184, 64200, 64216, 64232, 64249, 64265, 64281,\
 64297, 64313, 64329, 64345, 64362, 64378, 64394, 64409, 64425, 64441,\
 64457, 64473, 64489, 64505, 64520, 64536, 64552, 64568, 64583, 64599,\
 64615, 64630, 64646, 64661, 64677, 64693, 64708, 64723, 64739, 64754,\
 64770, 64785, 64800, 64816, 64831, 64846, 64862, 64877, 64892, 64907,\
 64922, 64938, 64953, 64968, 64983, 64998, 65013, 65028, 65043, 65058,\
 65073, 65088, 65103, 65117, 65132, 65147, 65162, 65177, 65191, 65206,\
 65221, 65236, 65250, 65265, 65280, 65294, 65309, 65323, 65338, 65352,\
 65367, 65381, 65396, 65410, 65425, 65439, 65453, 65468, 65482, 65497,\
 65511, 65525, 65539, 65554, 65568, 65582, 65596, 65610, 65624, 65639,\
 65653, 65667, 65681, 65695, 65709, 65723, 65737, 65751, 65765, 65779,\
 65793, 65806, 65820, 65834, 65848, 65862, 65876, 65889, 65903, 65917,\
 65930, 65944, 65958, 65971, 65985, 65999, 66012, 66026, 66039, 66053,\
 66067, 66080, 66093, 66107, 66120, 66134, 66147, 66161, 66174, 66187,\
 66201, 66214, 66227, 66241, 66254, 66267, 66280, 66294, 66307, 66320,\
 66333, 66346, 66359, 66373, 66386, 66399, 66412, 66425, 66438, 66451,\
 66464, 66477, 66490, 66503, 66516, 66529, 66542, 66554, 66567, 66580,\
 66593, 66606, 66619, 66631, 66644, 66657, 66670, 66682, 66695, 66708,\
 66720, 66733, 66746, 66758, 66771, 66783, 66796, 66809, 66821, 66834,\
 66846, 66859, 66871, 66884, 66896, 66908, 66921, 66933, 66946, 66958,\
 66970, 66983, 66995, 67007, 67020, 67032, 67044, 67056, 67069, 67081,\
 67093, 67105, 67117, 67130, 67142, 67154, 67166, 67178, 67190, 67202,\
 67214, 67226, 67238, 67250, 67262, 67274, 67286, 67298, 67310, 67322,\
 67334, 67346, 67358, 67370, 67382, 67393, 67405, 67417, 67429, 67441,\
 67452, 67464, 67476, 67488, 67499, 67511, 67523, 67534, 67546, 67558,\
 67569, 67581, 67593, 67604, 67616, 67627, 67639, 67650, 67662, 67673,\
 67685, 67696, 67708, 67719, 67731, 67742, 67754, 67765, 67776, 67788,\
 67799, 67811, 67822, 67833, 67845, 67856, 67867, 67878, 67890, 67901,\
 67912, 67923, 67935, 67946, 67957, 67968, 67979, 67991, 68002, 68013,\
 68024, 68035, 68046, 68057, 68068, 68079, 68090, 68101, 68112, 68123,\
 68134, 68145, 68156, 68167, 68178, 68189, 68200, 68211, 68222, 68233,\
 68244, 68255, 68265, 68276, 68287, 68298, 68309, 68320, 68330, 68341,\
 68352, 68363, 68373, 68384, 68395, 68405, 68416, 68427, 68437, 68448,\
 68459, 68469, 68480, 68491, 68501, 68512, 68522, 68533, 68544, 68554,\
 68565, 68575, 68586, 68596, 68607, 68617, 68628, 68638, 68648, 68659,\
 68669, 68680, 68690, 68701, 68711, 68721, 68732, 68742, 68752, 68763,\
 68773, 68783, 68794, 68804, 68814, 68824, 68835, 68845, 68855, 68865,\
 68876, 68886, 68896, 68906, 68916, 68926, 68937, 68947, 68957, 68967,\
 68977, 68987, 68997, 69007, 69017, 69027, 69037, 69048, 69058, 69068,\
 69078,
};

/* 1000 precomputed 1000 * log10(i) */
static SWINT_T swish_log10[] = {\
 0, 0, 3010, 4771, 6021, 6990, 7782, 8451, 9031, 9542,\
 10000, 10414, 10792, 11139, 11461, 11761, 12041, 12304, 12553, 12788,\
 13010, 13222, 13424, 13617, 13802, 13979, 14150, 14314, 14472, 14624,\
 14771, 14914, 15051, 15185, 15315, 15441, 15563, 15682, 15798, 15911,\
 16021, 16128, 16232, 16335, 16435, 16532, 16628, 16721, 16812, 16902,\
 16990, 17076, 17160, 17243, 17324, 17404, 17482, 17559, 17634, 17709,\
 17782, 17853, 17924, 17993, 18062, 18129, 18195, 18261, 18325, 18388,\
 18451, 18513, 18573, 18633, 18692, 18751, 18808, 18865, 18921, 18976,\
 19031, 19085, 19138, 19191, 19243, 19294, 19345, 19395, 19445, 19494,\
 19542, 19590, 19638, 19685, 19731, 19777, 19823, 19868, 19912, 19956,\
 20000, 20043, 20086, 20128, 20170, 20212, 20253, 20294, 20334, 20374,\
 20414, 20453, 20492, 20531, 20569, 20607, 20645, 20682, 20719, 20755,\
 20792, 20828, 20864, 20899, 20934, 20969, 21004, 21038, 21072, 21106,\
 21139, 21173, 21206, 21239, 21271, 21303, 21335, 21367, 21399, 21430,\
 21461, 21492, 21523, 21553, 21584, 21614, 21644, 21673, 21703, 21732,\
 21761, 21790, 21818, 21847, 21875, 21903, 21931, 21959, 21987, 22014,\
 22041, 22068, 22095, 22122, 22148, 22175, 22201, 22227, 22253, 22279,\
 22304, 22330, 22355, 22380, 22405, 22430, 22455, 22480, 22504, 22529,\
 22553, 22577, 22601, 22625, 22648, 22672, 22695, 22718, 22742, 22765,\
 22788, 22810, 22833, 22856, 22878, 22900, 22923, 22945, 22967, 22989,\
 23010, 23032, 23054, 23075, 23096, 23118, 23139, 23160, 23181, 23201,\
 23222, 23243, 23263, 23284, 23304, 23324, 23345, 23365, 23385, 23404,\
 23424, 23444, 23464, 23483, 23502, 23522, 23541, 23560, 23579, 23598,\
 23617, 23636, 23655, 23674, 23692, 23711, 23729, 23747, 23766, 23784,\
 23802, 23820, 23838, 23856, 23874, 23892, 23909, 23927, 23945, 23962,\
 23979, 23997, 24014, 24031, 24048, 24065, 24082, 24099, 24116, 24133,\
 24150, 24166, 24183, 24200, 24216, 24232, 24249, 24265, 24281, 24298,\
 24314, 24330, 24346, 24362, 24378, 24393, 24409, 24425, 24440, 24456,\
 24472, 24487, 24502, 24518, 24533, 24548, 24564, 24579, 24594, 24609,\
 24624, 24639, 24654, 24669, 24683, 24698, 24713, 24728, 24742, 24757,\
 24771, 24786, 24800, 24814, 24829, 24843, 24857, 24871, 24886, 24900,\
 24914, 24928, 24942, 24955, 24969, 24983, 24997, 25011, 25024, 25038,\
 25051, 25065, 25079, 25092, 25105, 25119, 25132, 25145, 25159, 25172,\
 25185, 25198, 25211, 25224, 25237, 25250, 25263, 25276, 25289, 25302,\
 25315, 25328, 25340, 25353, 25366, 25378, 25391, 25403, 25416, 25428,\
 25441, 25453, 25465, 25478, 25490, 25502, 25514, 25527, 25539, 25551,\
 25563, 25575, 25587, 25599, 25611, 25623, 25635, 25647, 25658, 25670,\
 25682, 25694, 25705, 25717, 25729, 25740, 25752, 25763, 25775, 25786,\
 25798, 25809, 25821, 25832, 25843, 25855, 25866, 25877, 25888, 25899,\
 25911, 25922, 25933, 25944, 25955, 25966, 25977, 25988, 25999, 26010,\
 26021, 26031, 26042, 26053, 26064, 26075, 26085, 26096, 26107, 26117,\
 26128, 26138, 26149, 26160, 26170, 26180, 26191, 26201, 26212, 26222,\
 26232, 26243, 26253, 26263, 26274, 26284, 26294, 26304, 26314, 26325,\
 26335, 26345, 26355, 26365, 26375, 26385, 26395, 26405, 26415, 26425,\
 26435, 26444, 26454, 26464, 26474, 26484, 26493, 26503, 26513, 26522,\
 26532, 26542, 26551, 26561, 26571, 26580, 26590, 26599, 26609, 26618,\
 26628, 26637, 26646, 26656, 26665, 26675, 26684, 26693, 26702, 26712,\
 26721, 26730, 26739, 26749, 26758, 26767, 26776, 26785, 26794, 26803,\
 26812, 26821, 26830, 26839, 26848, 26857, 26866, 26875, 26884, 26893,\
 26902, 26911, 26920, 26928, 26937, 26946, 26955, 26964, 26972, 26981,\
 26990, 26998, 27007, 27016, 27024, 27033, 27042, 27050, 27059, 27067,\
 27076, 27084, 27093, 27101, 27110, 27118, 27126, 27135, 27143, 27152,\
 27160, 27168, 27177, 27185, 27193, 27202, 27210, 27218, 27226, 27235,\
 27243, 27251, 27259, 27267, 27275, 27284, 27292, 27300, 27308, 27316,\
 27324, 27332, 27340, 27348, 27356, 27364, 27372, 27380, 27388, 27396,\
 27404, 27412, 27419, 27427, 27435, 27443, 27451, 27459, 27466, 27474,\
 27482, 27490, 27497, 27505, 27513, 27520, 27528, 27536, 27543, 27551,\
 27559, 27566, 27574, 27582, 27589, 27597, 27604, 27612, 27619, 27627,\
 27634, 27642, 27649, 27657, 27664, 27672, 27679, 27686, 27694, 27701,\
 27709, 27716, 27723, 27731, 27738, 27745, 27752, 27760, 27767, 27774,\
 27782, 27789, 27796, 27803, 27810, 27818, 27825, 27832, 27839, 27846,\
 27853, 27860, 27868, 27875, 27882, 27889, 27896, 27903, 27910, 27917,\
 27924, 27931, 27938, 27945, 27952, 27959, 27966, 27973, 27980, 27987,\
 27993, 28000, 28007, 28014, 28021, 28028, 28035, 28041, 28048, 28055,\
 28062, 28069, 28075, 28082, 28089, 28096, 28102, 28109, 28116, 28122,\
 28129, 28136, 28142, 28149, 28156, 28162, 28169, 28176, 28182, 28189,\
 28195, 28202, 28209, 28215, 28222, 28228, 28235, 28241, 28248, 28254,\
 28261, 28267, 28274, 28280, 28287, 28293, 28299, 28306, 28312, 28319,\
 28325, 28331, 28338, 28344, 28351, 28357, 28363, 28370, 28376, 28382,\
 28388, 28395, 28401, 28407, 28414, 28420, 28426, 28432, 28439, 28445,\
 28451, 28457, 28463, 28470, 28476, 28482, 28488, 28494, 28500, 28506,\
 28513, 28519, 28525, 28531, 28537, 28543, 28549, 28555, 28561, 28567,\
 28573, 28579, 28585, 28591, 28597, 28603, 28609, 28615, 28621, 28627,\
 28633, 28639, 28645, 28651, 28657, 28663, 28669, 28675, 28681, 28686,\
 28692, 28698, 28704, 28710, 28716, 28722, 28727, 28733, 28739, 28745,\
 28751, 28756, 28762, 28768, 28774, 28779, 28785, 28791, 28797, 28802,\
 28808, 28814, 28820, 28825, 28831, 28837, 28842, 28848, 28854, 28859,\
 28865, 28871, 28876, 28882, 28887, 28893, 28899, 28904, 28910, 28915,\
 28921, 28927, 28932, 28938, 28943, 28949, 28954, 28960, 28965, 28971,\
 28976, 28982, 28987, 28993, 28998, 29004, 29009, 29015, 29020, 29025,\
 29031, 29036, 29042, 29047, 29053, 29058, 29063, 29069, 29074, 29079,\
 29085, 29090, 29096, 29101, 29106, 29112, 29117, 29122, 29128, 29133,\
 29138, 29143, 29149, 29154, 29159, 29165, 29170, 29175, 29180, 29186,\
 29191, 29196, 29201, 29206, 29212, 29217, 29222, 29227, 29232, 29238,\
 29243, 29248, 29253, 29258, 29263, 29269, 29274, 29279, 29284, 29289,\
 29294, 29299, 29304, 29309, 29315, 29320, 29325, 29330, 29335, 29340,\
 29345, 29350, 29355, 29360, 29365, 29370, 29375, 29380, 29385, 29390,\
 29395, 29400, 29405, 29410, 29415, 29420, 29425, 29430, 29435, 29440,\
 29445, 29450, 29455, 29460, 29465, 29469, 29474, 29479, 29484, 29489,\
 29494, 29499, 29504, 29509, 29513, 29518, 29523, 29528, 29533, 29538,\
 29542, 29547, 29552, 29557, 29562, 29566, 29571, 29576, 29581, 29586,\
 29590, 29595, 29600, 29605, 29609, 29614, 29619, 29624, 29628, 29633,\
 29638, 29643, 29647, 29652, 29657, 29661, 29666, 29671, 29675, 29680,\
 29685, 29689, 29694, 29699, 29703, 29708, 29713, 29717, 29722, 29727,\
 29731, 29736, 29741, 29745, 29750, 29754, 29759, 29763, 29768, 29773,\
 29777, 29782, 29786, 29791, 29795, 29800, 29805, 29809, 29814, 29818,\
 29823, 29827, 29832, 29836, 29841, 29845, 29850, 29854, 29859, 29863,\
 29868, 29872, 29877, 29881, 29886, 29890, 29894, 29899, 29903, 29908,\
 29912, 29917, 29921, 29926, 29930, 29934, 29939, 29943, 29948, 29952,\
 29956, 29961, 29965, 29969, 29974, 29978, 29983, 29987, 29991, 29996,\
 30000,
};

typedef struct {
    SWINT_T    mask;
    SWINT_T    rank;
} RankFactor;

static RankFactor ranks[] = {
    {IN_TITLE,       RANK_TITLE},
    {IN_HEADER,      RANK_HEADER},
    {IN_META,        RANK_META},
    {IN_COMMENTS,    RANK_COMMENTS},
    {IN_EMPHASIZED,  RANK_EMPHASIZED}
};

#define numRanks (sizeof(ranks)/sizeof(ranks[0]))


/******************************************************************************
* build_struct_map
*
*   Builds an array to hold all possible structure values
*   (where the value is determined by the bits set in the structure flag)
*   This is just to provide a faster adjustment of rank based on structure.
*
*   A word's rank value is one plus the sum of the strucutre bit values.
*   The value of each structure bit is stored in the RankFactor array, and the
*   defaults for each RANK_* are in config.h.
*
*******************************************************************************/
static void build_struct_map( SWISH *sw )
{
    SWINT_T     structure;
    SWINT_T     i;

    SWINT_T     array_size = sizeof( sw->structure_map ) / sizeof( sw->structure_map[0]);

    for ( structure = 0; structure < array_size; structure++ )
    {
        SWINT_T factor = 1;  /* All words are of value 1 */

        for (i = 0; i < (SWINT_T)numRanks; i++)
            if (ranks[i].mask & structure)
                factor += ranks[i].rank;

        sw->structure_map[structure] = factor;
    }

    sw->structure_map_set = 1;  /* flag */
}

SWINT_T
getrank ( RESULT *r )
{
        SWISH   *sw;
        IndexFILE  *indexf;
        SWINT_T     scheme;

        indexf  = r->db_results->indexf;
        sw      = indexf->sw;
        scheme  = sw->RankScheme;

    if( DEBUG_RANK )
    {
        fprintf( stderr, "-----------------------------------------------------------------\n");
        fprintf( stderr, "Ranking Scheme: %lld \n", scheme );
    }

        switch ( scheme )
        {

           case 0:
           {
                return ( getrankDEF( r ) );
           }

           case 1:
           {
                if ( indexf->header.ignoreTotalWordCountWhenRanking ) {
                   fprintf(stderr, "IgnoreTotalWordCountWhenRanking must be 0 to use IDF ranking\n");
                   exit(1);
                }

                return ( getrankIDF( r ) );
           }

           default:
           {
                return ( getrankDEF( r ) );
           }

        }

}


/* 2001-11 jmruiz With thousands results (>1000000) this routine is a bottleneck.
**                (it is called thousands of times) Trying to avoid
**                this I have added some optimizations.
**                To avoid the annoying conversion in "return (SWINT_T)rank"
**                from double to SWINT_T that degrades performance search
**                I have switched to integer computations.
**
**                To avoid the loss of precision I use rank *10000,
**                reduction *10000, factor *10000, etc...
*/


/* renamed getrankDEF to allow for multiple schemes with generic case caller getrank()
karman Mon Aug 30 07:03:35 CDT 2004
*/


SWINT_T
getrankDEF( RESULT *r )
{
    SWUINT_T        *posdata;
    SWINT_T         meta_bias;
    IndexFILE  *indexf;
    SWINT_T         rank;
    SWINT_T         reduction;
    SWINT_T         words;  /* number of word positions (total words, not unique) in the given file -- probably should be per metaname */
    SWINT_T         i;
    SWISH      *sw;
    SWINT_T         metaID;
    SWINT_T         freq;
    SWINT_T         struct_tally[256];
    
    if( DEBUG_RANK )
    {
        for ( i = 0; i <= 255; i++ )
            struct_tally[i] = 0;
    }

    /* has rank already been calculated? */
    if ( r->rank >= 0 )
        return r->rank;

    /* load data locally */
    indexf  = r->db_results->indexf;
    sw      = indexf->sw;
    posdata = r->posdata;


    /* Get bias for the current metaID - metaID is stored in the rank for ease here */
    /* Currently, the rankbias is a number from -10 to +10.  It's an arbitrary range. */
    /* MetaBias is added to the word value for *each* word position, so it's multipied */
    /* times the number of words in the document (r->frequency). */

    metaID = r->rank * -1;
    meta_bias = indexf->header.metaEntryArray[ metaID - 1 ]->rank_bias;


    /* pre-build the structure map array */
    /* this maps a word's structure to a rank value */
    if ( !sw->structure_map_set )
    {
        build_struct_map( sw );
    }


    /* Add up the raw word values for each word found in the current document. */
    /* Notice the meta_bias added to each position */

    /* Might also bias words with low position values, for example */
    /* Should really consider r->tfrequency, which is the number of files that have */
    /* this word.  If the word is not found in many files then it should be ranked higher */

    rank = 1;
    freq = r->frequency;
    if ( freq > 100 )
        freq = 100;

    for(i = 0; i < freq; i++)
    {
        /* GET_STRUCTURE must return value in range! */
        rank += sw->structure_map[ GET_STRUCTURE(posdata[i]) ] + meta_bias;
        if( DEBUG_RANK > 1 )
        {
            fprintf(stderr, "Word entry %lld at position %lld has struct %lld\n", i,  GET_POSITION(posdata[i]),  GET_STRUCTURE(posdata[i]) );
            struct_tally[ GET_STRUCTURE(posdata[i]) ]++;
        }
    }

    if( DEBUG_RANK )
    {
        fprintf( stderr, "File num: %lld.  Raw Rank: %lld.  Frequency: %lld ", r->filenum, rank, r->frequency );
    }

    /* Ranks could end up less than zero -- but since the *final* rank is calcualted here */
    /* we can't know the *lowest* value to use an offset.  It might be better to track */
    /* the lowest value and delay actual final rank calculation/scaling to when the rank */
    /* is printed.  Especially when AND or OR'ing resutls. */

    if ( rank < 1 )
        rank = 1;

    rank = scale_word_score( rank );


    if( DEBUG_RANK > 1 )
    {
     fprintf( stderr, "scaled rank: %lld\n  Structure tally:\n", rank );

     for ( i = 0; i <= 255; i++ )
     {
         if ( struct_tally[i] )
         {
             fprintf( stderr, "      struct 0x%llx = count of %2lld (", i, struct_tally[i] );
            if ( i & IN_EMPHASIZED ) fprintf(stderr," EM");
            if ( i & IN_HEADER ) fprintf(stderr," HEADING");
            if ( i & IN_COMMENTS ) fprintf(stderr," COMMENT");
            if ( i & IN_META ) fprintf(stderr," META");
            if ( i & IN_BODY ) fprintf(stderr," BODY");
            if ( i & IN_HEAD ) fprintf(stderr," HEAD");
            if ( i & IN_TITLE ) fprintf(stderr," TITLE");
            if ( i & IN_FILE ) fprintf(stderr," FILE");
            fprintf(stderr," ) x rank map of %lld = %lld\n\n",  sw->structure_map[i], sw->structure_map[i] *  struct_tally[i]);
         }
     }
    }



    /* Return if IgnoreTotalWordCountWhenRanking is true (the default) */
    if ( indexf->header.ignoreTotalWordCountWhenRanking )
        return ( r->rank = rank / 100);




    /* bias by total words in the file -- this is off by default */

    /* if word count is significant, reduce rank by a number between 1.0 and 5.0 */
    words = getTotalWordsInFile( indexf, r->filenum );

    if (words <= 10)
        reduction = 10000;    /* 10000 * log10(10) = 10000 */
    else if (words > 1000)
    {
        if(words >= 100000)   /* log10(10000) is 5 */
            reduction = 50000;    /* As it was in previous version (5 * 10000) */
        
        /* rare case - do not overrun the static arrays (they only have 1000 entries) */
        else           
            reduction = (SWINT_T) (10000 * (floor(log10((double)words) + 0.5)));
    }
    else
        reduction = swish_log10[words];

    r->rank = (rank * 100) / reduction;

    return r->rank;
}


/* multiple ranking schemes allow for more fine-tuning as users require.

Use the -R <num> command line option or RankScheme() API method.

Default is to use getrankDEF() -- the same as -R 0

IDF ranking uses the total word frequency across all searched indexes
and a normalizing formula to negate effect of docs with different sizes.
The normalizing formula evaluates the word's density within a doc.
Greater density = greater relevance (higher rank)

NOTE that IgnoreTotalWordCountWhenRanking must be FALSE (0) in order to use
IDF ranking. Do error check on that config when calling getrankIDF()

TODO:
The ranking functions could likely be split up into smaller functions
since much code is shared.

karman Sun Aug 29 21:01:28 CDT 2004

*/


SWINT_T
getrankIDF( RESULT *r )
{

    SWUINT_T        *posdata;
    SWINT_T         meta_bias;
    IndexFILE  *indexf;
    SWINT_T         words;
    SWINT_T         i;
    SWISH      *sw;
    SWINT_T         metaID;
    SWINT_T         freq;
    SWINT_T         total_files;
    SWINT_T         total_words;
    SWINT_T         average_words;
    SWINT_T         density;
    SWINT_T         idf;
    SWINT_T         total_word_freq;
    SWINT_T         word_weight;
    SWINT_T         word_score;
    
    /* SWINT_T density_magic        = 2; */

    /* the value named 'rank' in getrank() is here named 'word_score'.
    it's largely semantic, but helps emphasize that *docs* are ranked,
    but *words* are scored. The doc rank is calculated based on the accrued word scores.

    However, the hash key name 'rank' is preserved in the r (RESULT) object
    for compatibility with getrank()

    */

/* this first part is identical to getrankDEF -- could be optimized as a single function */

    SWINT_T        struct_tally[256];
    if( DEBUG_RANK )
    {
        for ( i = 0; i <= 255; i++ )
            struct_tally[i] = 0;
    }


    if ( r->rank >= 0 )
        return r->rank;

    indexf  = r->db_results->indexf;
    sw      = indexf->sw;
    posdata = r->posdata;


    metaID = r->rank * -1;
    meta_bias = indexf->header.metaEntryArray[ metaID - 1 ]->rank_bias;

    if ( !sw->structure_map_set )
    {
        build_struct_map( sw );
    }
    
/* here we start to diverge */


    word_score  = 1;
    freq        = r->frequency;


    if( DEBUG_RANK )
    {
        fprintf( stderr, "File num: %lld  Word Score: %lld  Frequency: %lld  ", r->filenum, word_score, freq );
    }


    /* don't do this here; let density calc do it
    if ( freq > 100 )
        freq = 100;

    */

    /*
    IDF is the Inverse Document Frequency, or, the weight of the word in relationship to the
    collection of documents as a whole. Multiply the weight against the rank to give
    greater weight to words that appear less often in the collection.

    The biggest impact should be seen when OR'ing words together instead of AND'ing them.

    */

    total_files         = sw->TotalFiles;
    total_word_freq     = r->tfrequency;
    idf                 = (SWINT_T) ( log( total_files / total_word_freq ) * 1000 );

    /*  *1000 helps create a wider spread
    between the most common words and the rest of the pack:
    "word frequencies in natural language obey a power-law distribution" -- Maciej Ceglowski
    */

    if ( idf < 1 )
        idf = 1;
        /* only ubiquitous words like 'the' get idfs < 1.
           these should probably be stopwords anyway... 
        */


    if( DEBUG_RANK )
    {
        fprintf(stderr, "Total files: %lld   Total word freq: %lld   IDF: %lld  \n",
                 total_files, total_word_freq, idf );
    }

    /* calc word density. this normalizes document length so that longer docs
    don't rank higher out of sheer quantity. Hopefully this is a little more
    scientific than the out-of-the-air calc in getrankDEF() -- though effectiveness
    is likely the same... */

    words = getTotalWordsInFile( indexf, r->filenum );

    total_words         = sw->TotalWordPos;
    average_words       = total_words / total_files;

    if( DEBUG_RANK )
    {
        fprintf(stderr, "Total words: %lld   Average words: %lld   Indexed words in this doc: %lld  ",
         total_words, average_words, words );
    }


/* normalizing term density in a collection.

Amati & Van Rijsbergen "normalization 2" from
A Study of Parameter Tuning for Term Frequency Normalization
Ben HE
Department of Computing Science
University of Glasgow
Glasgow, UK
ben@dcs.gla.ac.uk

        term_freq_density = term_freq * log( 1 + c * av_doc_leng/doc_leng)

where c > 0 (optimized at 2 ... we think...)

 */

    /*

    density             = freq * log( 1 + ( density_magic * ( average_words / words ) ) ); */

    /* doesn't work that well with SWINT_T values. Use below (cruder) instead.
    NOTE that there is likely a sweet spot for density. A word like 'the' will always have a high
    density in normal language, but it is not very relevant. A word like 'foo' might have a very
    low density in doc A but slightly higher in doc B -- doc B is likely more relevant. So low
    density is not a good indicator and neither is high density. Instead, something like:

    | density  scale --->                  |
    0       X                           100
    useless sweet                       useless
    */


/* if for some reason there are 0 words in this document, we'll get a divide by zero
error in this next calculation. why might a doc have no words in it, and yet be in the 
set of matches we're evaluating? maybe because stopwords aren't counted? 
it's a mystery, just like mankind...
set words=1 if <1 so that we at least avoid the core dump -- would it be better to somehow
skip it altogether? throw an error? let's warn on stderr for now, just to alert the user
that something is awry. */

    if ( words < 1 ) {
    
        fprintf(stderr, "Word count for document %lld is zero\n", r->filenum );
        words = 1;
        
    }


    density             = ( ( average_words * 1000 ) / words ) * freq;

/* minimum density  */
    if (density < 1)
        density = 1;

/* scale word_weight back down by 100 or so, just to make it a little saner */

    word_weight         = ( density * idf ) / 100;


    if( DEBUG_RANK )
    {
        fprintf(stderr, "Density: %lld    Word Weight: %lld   \n", density, word_weight );
    }


    for(i = 0; i < freq; i++)
    {
        /* GET_STRUCTURE must return value in range! */

        word_score += word_weight * ( sw->structure_map[ GET_STRUCTURE(posdata[i]) ] + meta_bias );

        if( DEBUG_RANK > 1 )
        {
            fprintf(stderr, "Word entry %lld at position %lld has struct %lld\n", i,  GET_POSITION(posdata[i]),  GET_STRUCTURE(posdata[i]) );

            struct_tally[ GET_STRUCTURE(posdata[i]) ]++;
        }

    }

    /* see comment in getrank() about why we make sure score is positive and non-zero */

    if ( word_score < 1 )
        word_score = 1;



    if( DEBUG_RANK )
    {
        fprintf(stderr, "Raw score after IDF weighting: %lld  \n", word_score );
    }

    word_score = scale_word_score( word_score );

    if( DEBUG_RANK > 1 )
    {
        fprintf( stderr, "scaled rank: %lld\n  Structure tally:\n", word_score );

        for ( i = 0; i <= 255; i++ )
        {
            if ( struct_tally[i] )
            {
                fprintf( stderr, "      struct 0x%llx = count of %2lld (", i, struct_tally[i] );
                if ( i & IN_EMPHASIZED ) fprintf(stderr," EM");
                if ( i & IN_HEADER ) fprintf(stderr," HEADING");
                if ( i & IN_COMMENTS ) fprintf(stderr," COMMENT");
                if ( i & IN_META ) fprintf(stderr," META");
                if ( i & IN_BODY ) fprintf(stderr," BODY");
                if ( i & IN_HEAD ) fprintf(stderr," HEAD");
                if ( i & IN_TITLE ) fprintf(stderr," TITLE");
                if ( i & IN_FILE ) fprintf(stderr," FILE");
                fprintf(stderr," ) x rank map of %lld = %lld\n\n",  sw->structure_map[i], sw->structure_map[i] *  struct_tally[i]);
            }
        }
    }
    
    if ( DEBUG_RANK )
    {
        fprintf(stderr, "Scaled score: %lld \n", word_score );
        
    }

    return ( r->rank = word_score );
}

SWINT_T
scale_word_score( SWINT_T score )
{

    /* Scale the rank - this was originally based on frequency */
    /* Uses lookup tables for values <= 1000, otherwise calculate */


        return  score > 1000
                ? (SWINT_T) floor( (log((double)score) * 10000 ) + 0.5)
                : swish_log[score];

}
