// sine and cosine of the Earth's tilt
#define COS_ALPHA 0.917060074385124 // cos(23.5)
#define SIN_ALPHA 0.398749068925246 // sin(23.5)

const float THETA_TABLE[] = {1.000000,0.999577,0.998308,0.996195,0.993238,0.989442,0.984808,0.979341,0.973045,0.965926,0.957990,0.949243,0.939693,0.929348,0.918216,0.906308,0.893633,0.880201,0.866025,0.851117,0.835488,0.819152,0.802123,0.784416,0.766044,0.747025,0.727374,0.707107,0.686242,0.664796,0.642788,0.620235,0.597159,0.573576,0.549509,0.524977,0.500000,0.474600,0.448799,0.422618,0.396080,0.369206,0.342020,0.314545,0.286803,0.258819,0.230616,0.202218,0.173648,0.144932,0.116093,0.087156,0.058145,0.029085,0.000000,-0.029085,-0.058145,-0.087156,-0.116093,-0.144932,-0.173648,-0.202218,-0.230616,-0.258819,-0.286803,-0.314545,-0.342020,-0.369206,-0.396080,-0.422618,-0.448799,-0.474600,-0.500000,-0.524977,-0.549509,-0.573576,-0.597159,-0.620235,-0.642788,-0.664796,-0.686242,-0.707107,-0.727374,-0.747025,-0.766044,-0.784416,-0.802123,-0.819152,-0.835488,-0.851117,-0.866025,-0.880201,-0.893633,-0.906308,-0.918216,-0.929348,-0.939693,-0.949243,-0.957990,-0.965926,-0.973045,-0.979341,-0.984808,-0.989442,-0.993238,-0.996195,-0.998308,-0.999577,-1.000000,-0.999577,-0.998308,-0.996195,-0.993238,-0.989442,-0.984808,-0.979341,-0.973045,-0.965926,-0.957990,-0.949243,-0.939693,-0.929348,-0.918216,-0.906308,-0.893633,-0.880201,-0.866025,-0.851117,-0.835488,-0.819152,-0.802123,-0.784416,-0.766044,-0.747025,-0.727374,-0.707107,-0.686242,-0.664796,-0.642788,-0.620235,-0.597159,-0.573576,-0.549509,-0.524977,-0.500000,-0.474600,-0.448799,-0.422618,-0.396080,-0.369206,-0.342020,-0.314545,-0.286803,-0.258819,-0.230616,-0.202218,-0.173648,-0.144932,-0.116093,-0.087156,-0.058145,-0.029085,-0.000000,0.029085,0.058145,0.087156,0.116093,0.144932,0.173648,0.202218,0.230616,0.258819,0.286803,0.314545,0.342020,0.369206,0.396080,0.422618,0.448799,0.474600,0.500000,0.524977,0.549509,0.573576,0.597159,0.620235,0.642788,0.664796,0.686242,0.707107,0.727374,0.747025,0.766044,0.784416,0.802123,0.819152,0.835488,0.851117,0.866025,0.880201,0.893633,0.906308,0.918216,0.929348,0.939693,0.949243,0.957990,0.965926,0.973045,0.979341,0.984808,0.989442,0.993238,0.996195,0.998308,0.999577};

const float PHI_TABLE[] = {0.000000,0.018699,0.037391,0.056070,0.074730,0.093364,0.111964,0.130526,0.149042,0.167506,0.185912,0.204252,0.222521,0.240712,0.258819,0.276836,0.294755,0.312572,0.330279,0.347871,0.365341,0.382683,0.399892,0.416961,0.433884,0.450655,0.467269,0.483719,0.500000,0.516106,0.532032,0.547772,0.563320,0.578671,0.593820,0.608761,0.623490,0.638000,0.652287,0.666347,0.680173,0.693761,0.707107,0.720205,0.733052,0.745642,0.757972,0.770036,0.781831,0.793353,0.804598,0.815561,0.826239,0.836628,0.846724,0.856525,0.866025,0.875223,0.884115,0.892698,0.900969,0.908924,0.916562,0.923880,0.930874,0.937542,0.943883,0.949894,0.955573,0.960917,0.965926,0.970597,0.974928,0.978918,0.982566,0.985871,0.988831,0.991445,0.993712,0.995632,0.997204,0.998427,0.999301,0.999825,1.000000,0.999825,0.999301,0.998427,0.997204,0.995632,0.993712,0.991445,0.988831,0.985871,0.982566,0.978918,0.974928,0.970597,0.965926,0.960917,0.955573,0.949894,0.943883,0.937542,0.930874,0.923880,0.916562,0.908924,0.900969,0.892698,0.884115,0.875223,0.866025,0.856525,0.846724,0.836628,0.826239,0.815561,0.804598,0.793353,0.781831,0.770036,0.757972,0.745642,0.733052,0.720205,0.707107,0.693761,0.680173,0.666347,0.652287,0.638000,0.623490,0.608761,0.593820,0.578671,0.563320,0.547772,0.532032,0.516106,0.500000,0.483719,0.467269,0.450655,0.433884,0.416961,0.399892,0.382683,0.365341,0.347871,0.330279,0.312572,0.294755,0.276836,0.258819,0.240712,0.222521,0.204252,0.185912,0.167506,0.149042,0.130526,0.111964,0.093364,0.074730,0.056070,0.037391,0.018699};

const float YEAR_TABLE[] = {0.985220,0.982126,0.978740,0.975065,0.971100,0.966848,0.962309,0.957485,0.952378,0.946988,0.941317,0.935368,0.929141,0.922640,0.915864,0.908818,0.901502,0.893919,0.886071,0.877960,0.869589,0.860961,0.852078,0.842942,0.833556,0.823923,0.814046,0.803928,0.793572,0.782980,0.772157,0.761104,0.749826,0.738326,0.726608,0.714673,0.702527,0.690173,0.677615,0.664855,0.651899,0.638749,0.625411,0.611886,0.598181,0.584298,0.570242,0.556017,0.541628,0.527078,0.512371,0.497513,0.482508,0.467359,0.452072,0.436651,0.421101,0.405426,0.389630,0.373720,0.357698,0.341571,0.325342,0.309017,0.292600,0.276097,0.259512,0.242850,0.226116,0.209315,0.192452,0.175531,0.158559,0.141540,0.124479,0.107381,0.090252,0.073095,0.055917,0.038722,0.021516,0.004304,-0.012910,-0.030120,-0.047321,-0.064508,-0.081676,-0.098820,-0.115935,-0.133015,-0.150055,-0.167052,-0.183998,-0.200891,-0.217723,-0.234491,-0.251190,-0.267814,-0.284359,-0.300820,-0.317191,-0.333469,-0.349647,-0.365723,-0.381689,-0.397543,-0.413279,-0.428892,-0.444378,-0.459733,-0.474951,-0.490029,-0.504961,-0.519744,-0.534373,-0.548843,-0.563151,-0.577292,-0.591261,-0.605056,-0.618671,-0.632103,-0.645348,-0.658402,-0.671260,-0.683919,-0.696376,-0.708627,-0.720667,-0.732494,-0.744104,-0.755493,-0.766659,-0.777597,-0.788305,-0.798779,-0.809017,-0.819015,-0.828770,-0.838280,-0.847541,-0.856551,-0.865307,-0.873807,-0.882048,-0.890028,-0.897743,-0.905193,-0.912375,-0.919286,-0.925925,-0.932289,-0.938377,-0.944188,-0.949718,-0.954967,-0.959933,-0.964614,-0.969010,-0.973118,-0.976938,-0.980469,-0.983709,-0.986658,-0.989314,-0.991677,-0.993747,-0.995521,-0.997001,-0.998186,-0.999074,-0.999667,-0.999963,-0.999963,-0.999667,-0.999074,-0.998186,-0.997001,-0.995521,-0.993747,-0.991677,-0.989314,-0.986658,-0.983709,-0.980469,-0.976938,-0.973118,-0.969010,-0.964614,-0.959933,-0.954967,-0.949718,-0.944188,-0.938377,-0.932289,-0.925925,-0.919286,-0.912375,-0.905193,-0.897743,-0.890028,-0.882048,-0.873807,-0.865307,-0.856551,-0.847541,-0.838280,-0.828770,-0.819015,-0.809017,-0.798779,-0.788305,-0.777597,-0.766659,-0.755493,-0.744104,-0.732494,-0.720667,-0.708627,-0.696376,-0.683919,-0.671260,-0.658402,-0.645348,-0.632103,-0.618671,-0.605056,-0.591261,-0.577292,-0.563151,-0.548843,-0.534373,-0.519744,-0.504961,-0.490029,-0.474951,-0.459733,-0.444378,-0.428892,-0.413279,-0.397543,-0.381689,-0.365723,-0.349647,-0.333469,-0.317191,-0.300820,-0.284359,-0.267814,-0.251190,-0.234491,-0.217723,-0.200891,-0.183998,-0.167052,-0.150055,-0.133015,-0.115935,-0.098820,-0.081676,-0.064508,-0.047321,-0.030120,-0.012910,0.004304,0.021516,0.038722,0.055917,0.073095,0.090252,0.107381,0.124479,0.141540,0.158559,0.175531,0.192452,0.209315,0.226116,0.242850,0.259512,0.276097,0.292600,0.309017,0.325342,0.341571,0.357698,0.373720,0.389630,0.405426,0.421101,0.436651,0.452072,0.467359,0.482508,0.497513,0.512371,0.527078,0.541628,0.556017,0.570242,0.584298,0.598181,0.611886,0.625411,0.638749,0.651899,0.664855,0.677615,0.690173,0.702527,0.714673,0.726608,0.738326,0.749826,0.761104,0.772157,0.782980,0.793572,0.803928,0.814046,0.823923,0.833556,0.842942,0.852078,0.860961,0.869589,0.877960,0.886071,0.893919,0.901502,0.908818,0.915864,0.922640,0.929141,0.935368,0.941317,0.946988,0.952378,0.957485,0.962309,0.966848,0.971100,0.975065,0.978740,0.982126,0.985220,0.988023,0.990532,0.992749,0.994671,0.996298,0.997630,0.998667,0.999407,0.999852,1.000000,0.999852,0.999407,0.998667,0.997630,0.996298,0.994671,0.992749,0.990532,0.988023};