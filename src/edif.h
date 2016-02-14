/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
	 /* Put the tokens into the symbol table, so that GDB and other debuggers
	    know about them.  */
enum yytokentype {
	EDIF_TOK_IDENT = 258,
	EDIF_TOK_INT = 259,
	EDIF_TOK_KEYWORD = 260,
	EDIF_TOK_STR = 261,
	EDIF_TOK_ANGLE = 262,
	EDIF_TOK_BEHAVIOR = 263,
	EDIF_TOK_CALCULATED = 264,
	EDIF_TOK_CAPACITANCE = 265,
	EDIF_TOK_CENTERCENTER = 266,
	EDIF_TOK_CENTERLEFT = 267,
	EDIF_TOK_CENTERRIGHT = 268,
	EDIF_TOK_CHARGE = 269,
	EDIF_TOK_CONDUCTANCE = 270,
	EDIF_TOK_CURRENT = 271,
	EDIF_TOK_DISTANCE = 272,
	EDIF_TOK_DOCUMENT = 273,
	EDIF_TOK_ENERGY = 274,
	EDIF_TOK_EXTEND = 275,
	EDIF_TOK_FLUX = 276,
	EDIF_TOK_FREQUENCY = 277,
	EDIF_TOK_GENERIC = 278,
	EDIF_TOK_GRAPHIC = 279,
	EDIF_TOK_INDUCTANCE = 280,
	EDIF_TOK_INOUT = 281,
	EDIF_TOK_INPUT = 282,
	EDIF_TOK_LOGICMODEL = 283,
	EDIF_TOK_LOWERCENTER = 284,
	EDIF_TOK_LOWERLEFT = 285,
	EDIF_TOK_LOWERRIGHT = 286,
	EDIF_TOK_MASKLAYOUT = 287,
	EDIF_TOK_MASS = 288,
	EDIF_TOK_MEASURED = 289,
	EDIF_TOK_MX = 290,
	EDIF_TOK_MXR90 = 291,
	EDIF_TOK_MY = 292,
	EDIF_TOK_MYR90 = 293,
	EDIF_TOK_NETLIST = 294,
	EDIF_TOK_OUTPUT = 295,
	EDIF_TOK_PCBLAYOUT = 296,
	EDIF_TOK_POWER = 297,
	EDIF_TOK_R0 = 298,
	EDIF_TOK_R180 = 299,
	EDIF_TOK_R270 = 300,
	EDIF_TOK_R90 = 301,
	EDIF_TOK_REQUIRED = 302,
	EDIF_TOK_RESISTANCE = 303,
	EDIF_TOK_RIPPER = 304,
	EDIF_TOK_ROUND = 305,
	EDIF_TOK_SCHEMATIC = 306,
	EDIF_TOK_STRANGER = 307,
	EDIF_TOK_SYMBOLIC = 308,
	EDIF_TOK_TEMPERATURE = 309,
	EDIF_TOK_TIE = 310,
	EDIF_TOK_TIME = 311,
	EDIF_TOK_TRUNCATE = 312,
	EDIF_TOK_UPPERCENTER = 313,
	EDIF_TOK_UPPERLEFT = 314,
	EDIF_TOK_UPPERRIGHT = 315,
	EDIF_TOK_VOLTAGE = 316,
	EDIF_TOK_ACLOAD = 317,
	EDIF_TOK_AFTER = 318,
	EDIF_TOK_ANNOTATE = 319,
	EDIF_TOK_APPLY = 320,
	EDIF_TOK_ARC = 321,
	EDIF_TOK_ARRAY = 322,
	EDIF_TOK_ARRAYMACRO = 323,
	EDIF_TOK_ARRAYRELATEDINFO = 324,
	EDIF_TOK_ARRAYSITE = 325,
	EDIF_TOK_ATLEAST = 326,
	EDIF_TOK_ATMOST = 327,
	EDIF_TOK_AUTHOR = 328,
	EDIF_TOK_BASEARRAY = 329,
	EDIF_TOK_BECOMES = 330,
	EDIF_TOK_BETWEEN = 331,
	EDIF_TOK_BOOLEAN = 332,
	EDIF_TOK_BOOLEANDISPLAY = 333,
	EDIF_TOK_BOOLEANMAP = 334,
	EDIF_TOK_BORDERPATTERN = 335,
	EDIF_TOK_BORDERWIDTH = 336,
	EDIF_TOK_BOUNDINGBOX = 337,
	EDIF_TOK_CELL = 338,
	EDIF_TOK_CELLREF = 339,
	EDIF_TOK_CELLTYPE = 340,
	EDIF_TOK_CHANGE = 341,
	EDIF_TOK_CIRCLE = 342,
	EDIF_TOK_COLOR = 343,
	EDIF_TOK_COMMENT = 344,
	EDIF_TOK_COMMENTGRAPHICS = 345,
	EDIF_TOK_COMPOUND = 346,
	EDIF_TOK_CONNECTLOCATION = 347,
	EDIF_TOK_CONTENTS = 348,
	EDIF_TOK_CORNERTYPE = 349,
	EDIF_TOK_CRITICALITY = 350,
	EDIF_TOK_CURRENTMAP = 351,
	EDIF_TOK_CURVE = 352,
	EDIF_TOK_CYCLE = 353,
	EDIF_TOK_DATAORIGIN = 354,
	EDIF_TOK_DCFANINLOAD = 355,
	EDIF_TOK_DCFANOUTLOAD = 356,
	EDIF_TOK_DCMAXFANIN = 357,
	EDIF_TOK_DCMAXFANOUT = 358,
	EDIF_TOK_DELAY = 359,
	EDIF_TOK_DELTA = 360,
	EDIF_TOK_DERIVATION = 361,
	EDIF_TOK_DESIGN = 362,
	EDIF_TOK_DESIGNATOR = 363,
	EDIF_TOK_DIFFERENCE = 364,
	EDIF_TOK_DIRECTION = 365,
	EDIF_TOK_DISPLAY = 366,
	EDIF_TOK_DOMINATES = 367,
	EDIF_TOK_DOT = 368,
	EDIF_TOK_DURATION = 369,
	EDIF_TOK_E = 370,
	EDIF_TOK_EDIF = 371,
	EDIF_TOK_EDIFLEVEL = 372,
	EDIF_TOK_EDIFVERSION = 373,
	EDIF_TOK_ENCLOSUREDISTANCE = 374,
	EDIF_TOK_ENDTYPE = 375,
	EDIF_TOK_ENTRY = 376,
	EDIF_TOK_EVENT = 377,
	EDIF_TOK_EXACTLY = 378,
	EDIF_TOK_EXTERNAL = 379,
	EDIF_TOK_FABRICATE = 380,
	EDIF_TOK_FALSE = 381,
	EDIF_TOK_FIGURE = 382,
	EDIF_TOK_FIGUREAREA = 383,
	EDIF_TOK_FIGUREGROUP = 384,
	EDIF_TOK_FIGUREGROUPOBJECT = 385,
	EDIF_TOK_FIGUREGROUPOVERRIDE = 386,
	EDIF_TOK_FIGUREGROUPREF = 387,
	EDIF_TOK_FIGUREPERIMETER = 388,
	EDIF_TOK_FIGUREWIDTH = 389,
	EDIF_TOK_FILLPATTERN = 390,
	EDIF_TOK_FOLLOW = 391,
	EDIF_TOK_FORBIDDENEVENT = 392,
	EDIF_TOK_GLOBALPORTREF = 393,
	EDIF_TOK_GREATERTHAN = 394,
	EDIF_TOK_GRIDMAP = 395,
	EDIF_TOK_IGNORE = 396,
	EDIF_TOK_INCLUDEFIGUREGROUP = 397,
	EDIF_TOK_INITIAL = 398,
	EDIF_TOK_INSTANCE = 399,
	EDIF_TOK_INSTANCEBACKANNOTATE = 400,
	EDIF_TOK_INSTANCEGROUP = 401,
	EDIF_TOK_INSTANCEMAP = 402,
	EDIF_TOK_INSTANCEREF = 403,
	EDIF_TOK_INTEGER = 404,
	EDIF_TOK_INTEGERDISPLAY = 405,
	EDIF_TOK_INTERFACE = 406,
	EDIF_TOK_INTERFIGUREGROUPSPACING = 407,
	EDIF_TOK_INTERSECTION = 408,
	EDIF_TOK_INTRAFIGUREGROUPSPACING = 409,
	EDIF_TOK_INVERSE = 410,
	EDIF_TOK_ISOLATED = 411,
	EDIF_TOK_JOINED = 412,
	EDIF_TOK_JUSTIFY = 413,
	EDIF_TOK_KEYWORDDISPLAY = 414,
	EDIF_TOK_KEYWORDLEVEL = 415,
	EDIF_TOK_KEYWORDMAP = 416,
	EDIF_TOK_LESSTHAN = 417,
	EDIF_TOK_LIBRARY = 418,
	EDIF_TOK_LIBRARYREF = 419,
	EDIF_TOK_LISTOFNETS = 420,
	EDIF_TOK_LISTOFPORTS = 421,
	EDIF_TOK_LOADDELAY = 422,
	EDIF_TOK_LOGICASSIGN = 423,
	EDIF_TOK_LOGICINPUT = 424,
	EDIF_TOK_LOGICLIST = 425,
	EDIF_TOK_LOGICMAPINPUT = 426,
	EDIF_TOK_LOGICMAPOUTPUT = 427,
	EDIF_TOK_LOGICONEOF = 428,
	EDIF_TOK_LOGICOUTPUT = 429,
	EDIF_TOK_LOGICPORT = 430,
	EDIF_TOK_LOGICREF = 431,
	EDIF_TOK_LOGICVALUE = 432,
	EDIF_TOK_LOGICWAVEFORM = 433,
	EDIF_TOK_MAINTAIN = 434,
	EDIF_TOK_MATCH = 435,
	EDIF_TOK_MEMBER = 436,
	EDIF_TOK_MINOMAX = 437,
	EDIF_TOK_MINOMAXDISPLAY = 438,
	EDIF_TOK_MNM = 439,
	EDIF_TOK_MULTIPLEVALUESET = 440,
	EDIF_TOK_MUSTJOIN = 441,
	EDIF_TOK_NAME = 442,
	EDIF_TOK_NET = 443,
	EDIF_TOK_NETBACKANNOTATE = 444,
	EDIF_TOK_NETBUNDLE = 445,
	EDIF_TOK_NETDELAY = 446,
	EDIF_TOK_NETGROUP = 447,
	EDIF_TOK_NETMAP = 448,
	EDIF_TOK_NETREF = 449,
	EDIF_TOK_NOCHANGE = 450,
	EDIF_TOK_NONPERMUTABLE = 451,
	EDIF_TOK_NOTALLOWED = 452,
	EDIF_TOK_NOTCHSPACING = 453,
	EDIF_TOK_NUMBER = 454,
	EDIF_TOK_NUMBERDEFINITION = 455,
	EDIF_TOK_NUMBERDISPLAY = 456,
	EDIF_TOK_OFFPAGECONNECTOR = 457,
	EDIF_TOK_OFFSETEVENT = 458,
	EDIF_TOK_OPENSHAPE = 459,
	EDIF_TOK_ORIENTATION = 460,
	EDIF_TOK_ORIGIN = 461,
	EDIF_TOK_OVERHANGDISTANCE = 462,
	EDIF_TOK_OVERLAPDISTANCE = 463,
	EDIF_TOK_OVERSIZE = 464,
	EDIF_TOK_OWNER = 465,
	EDIF_TOK_PAGE = 466,
	EDIF_TOK_PAGESIZE = 467,
	EDIF_TOK_PARAMETER = 468,
	EDIF_TOK_PARAMETERASSIGN = 469,
	EDIF_TOK_PARAMETERDISPLAY = 470,
	EDIF_TOK_PATH = 471,
	EDIF_TOK_PATHDELAY = 472,
	EDIF_TOK_PATHWIDTH = 473,
	EDIF_TOK_PERMUTABLE = 474,
	EDIF_TOK_PHYSICALDESIGNRULE = 475,
	EDIF_TOK_PLUG = 476,
	EDIF_TOK_POINT = 477,
	EDIF_TOK_POINTDISPLAY = 478,
	EDIF_TOK_POINTLIST = 479,
	EDIF_TOK_POLYGON = 480,
	EDIF_TOK_PORT = 481,
	EDIF_TOK_PORTBACKANNOTATE = 482,
	EDIF_TOK_PORTBUNDLE = 483,
	EDIF_TOK_PORTDELAY = 484,
	EDIF_TOK_PORTGROUP = 485,
	EDIF_TOK_PORTIMPLEMENTATION = 486,
	EDIF_TOK_PORTINSTANCE = 487,
	EDIF_TOK_PORTLIST = 488,
	EDIF_TOK_PORTLISTALIAS = 489,
	EDIF_TOK_PORTMAP = 490,
	EDIF_TOK_PORTREF = 491,
	EDIF_TOK_PROGRAM = 492,
	EDIF_TOK_PROPERTY = 493,
	EDIF_TOK_PROPERTYDISPLAY = 494,
	EDIF_TOK_PROTECTIONFRAME = 495,
	EDIF_TOK_PT = 496,
	EDIF_TOK_RANGEVECTOR = 497,
	EDIF_TOK_RECTANGLE = 498,
	EDIF_TOK_RECTANGLESIZE = 499,
	EDIF_TOK_RENAME = 500,
	EDIF_TOK_RESOLVES = 501,
	EDIF_TOK_SCALE = 502,
	EDIF_TOK_SCALEX = 503,
	EDIF_TOK_SCALEY = 504,
	EDIF_TOK_SECTION = 505,
	EDIF_TOK_SHAPE = 506,
	EDIF_TOK_SIMULATE = 507,
	EDIF_TOK_SIMULATIONINFO = 508,
	EDIF_TOK_SINGLEVALUESET = 509,
	EDIF_TOK_SITE = 510,
	EDIF_TOK_SOCKET = 511,
	EDIF_TOK_SOCKETSET = 512,
	EDIF_TOK_STATUS = 513,
	EDIF_TOK_STEADY = 514,
	EDIF_TOK_STRING = 515,
	EDIF_TOK_STRINGDISPLAY = 516,
	EDIF_TOK_STRONG = 517,
	EDIF_TOK_SYMBOL = 518,
	EDIF_TOK_SYMMETRY = 519,
	EDIF_TOK_TABLE = 520,
	EDIF_TOK_TABLEDEFAULT = 521,
	EDIF_TOK_TECHNOLOGY = 522,
	EDIF_TOK_TEXTHEIGHT = 523,
	EDIF_TOK_TIMEINTERVAL = 524,
	EDIF_TOK_TIMESTAMP = 525,
	EDIF_TOK_TIMING = 526,
	EDIF_TOK_TRANSFORM = 527,
	EDIF_TOK_TRANSITION = 528,
	EDIF_TOK_TRIGGER = 529,
	EDIF_TOK_TRUE = 530,
	EDIF_TOK_UNCONSTRAINED = 531,
	EDIF_TOK_UNDEFINED = 532,
	EDIF_TOK_UNION = 533,
	EDIF_TOK_UNIT = 534,
	EDIF_TOK_UNUSED = 535,
	EDIF_TOK_USERDATA = 536,
	EDIF_TOK_VERSION = 537,
	EDIF_TOK_VIEW = 538,
	EDIF_TOK_VIEWLIST = 539,
	EDIF_TOK_VIEWMAP = 540,
	EDIF_TOK_VIEWREF = 541,
	EDIF_TOK_VIEWTYPE = 542,
	EDIF_TOK_VISIBLE = 543,
	EDIF_TOK_VOLTAGEMAP = 544,
	EDIF_TOK_WAVEVALUE = 545,
	EDIF_TOK_WEAK = 546,
	EDIF_TOK_WEAKJOINED = 547,
	EDIF_TOK_WHEN = 548,
	EDIF_TOK_WRITTEN = 549
};
#endif
/* Tokens.  */
#define EDIF_TOK_IDENT 258
#define EDIF_TOK_INT 259
#define EDIF_TOK_KEYWORD 260
#define EDIF_TOK_STR 261
#define EDIF_TOK_ANGLE 262
#define EDIF_TOK_BEHAVIOR 263
#define EDIF_TOK_CALCULATED 264
#define EDIF_TOK_CAPACITANCE 265
#define EDIF_TOK_CENTERCENTER 266
#define EDIF_TOK_CENTERLEFT 267
#define EDIF_TOK_CENTERRIGHT 268
#define EDIF_TOK_CHARGE 269
#define EDIF_TOK_CONDUCTANCE 270
#define EDIF_TOK_CURRENT 271
#define EDIF_TOK_DISTANCE 272
#define EDIF_TOK_DOCUMENT 273
#define EDIF_TOK_ENERGY 274
#define EDIF_TOK_EXTEND 275
#define EDIF_TOK_FLUX 276
#define EDIF_TOK_FREQUENCY 277
#define EDIF_TOK_GENERIC 278
#define EDIF_TOK_GRAPHIC 279
#define EDIF_TOK_INDUCTANCE 280
#define EDIF_TOK_INOUT 281
#define EDIF_TOK_INPUT 282
#define EDIF_TOK_LOGICMODEL 283
#define EDIF_TOK_LOWERCENTER 284
#define EDIF_TOK_LOWERLEFT 285
#define EDIF_TOK_LOWERRIGHT 286
#define EDIF_TOK_MASKLAYOUT 287
#define EDIF_TOK_MASS 288
#define EDIF_TOK_MEASURED 289
#define EDIF_TOK_MX 290
#define EDIF_TOK_MXR90 291
#define EDIF_TOK_MY 292
#define EDIF_TOK_MYR90 293
#define EDIF_TOK_NETLIST 294
#define EDIF_TOK_OUTPUT 295
#define EDIF_TOK_PCBLAYOUT 296
#define EDIF_TOK_POWER 297
#define EDIF_TOK_R0 298
#define EDIF_TOK_R180 299
#define EDIF_TOK_R270 300
#define EDIF_TOK_R90 301
#define EDIF_TOK_REQUIRED 302
#define EDIF_TOK_RESISTANCE 303
#define EDIF_TOK_RIPPER 304
#define EDIF_TOK_ROUND 305
#define EDIF_TOK_SCHEMATIC 306
#define EDIF_TOK_STRANGER 307
#define EDIF_TOK_SYMBOLIC 308
#define EDIF_TOK_TEMPERATURE 309
#define EDIF_TOK_TIE 310
#define EDIF_TOK_TIME 311
#define EDIF_TOK_TRUNCATE 312
#define EDIF_TOK_UPPERCENTER 313
#define EDIF_TOK_UPPERLEFT 314
#define EDIF_TOK_UPPERRIGHT 315
#define EDIF_TOK_VOLTAGE 316
#define EDIF_TOK_ACLOAD 317
#define EDIF_TOK_AFTER 318
#define EDIF_TOK_ANNOTATE 319
#define EDIF_TOK_APPLY 320
#define EDIF_TOK_ARC 321
#define EDIF_TOK_ARRAY 322
#define EDIF_TOK_ARRAYMACRO 323
#define EDIF_TOK_ARRAYRELATEDINFO 324
#define EDIF_TOK_ARRAYSITE 325
#define EDIF_TOK_ATLEAST 326
#define EDIF_TOK_ATMOST 327
#define EDIF_TOK_AUTHOR 328
#define EDIF_TOK_BASEARRAY 329
#define EDIF_TOK_BECOMES 330
#define EDIF_TOK_BETWEEN 331
#define EDIF_TOK_BOOLEAN 332
#define EDIF_TOK_BOOLEANDISPLAY 333
#define EDIF_TOK_BOOLEANMAP 334
#define EDIF_TOK_BORDERPATTERN 335
#define EDIF_TOK_BORDERWIDTH 336
#define EDIF_TOK_BOUNDINGBOX 337
#define EDIF_TOK_CELL 338
#define EDIF_TOK_CELLREF 339
#define EDIF_TOK_CELLTYPE 340
#define EDIF_TOK_CHANGE 341
#define EDIF_TOK_CIRCLE 342
#define EDIF_TOK_COLOR 343
#define EDIF_TOK_COMMENT 344
#define EDIF_TOK_COMMENTGRAPHICS 345
#define EDIF_TOK_COMPOUND 346
#define EDIF_TOK_CONNECTLOCATION 347
#define EDIF_TOK_CONTENTS 348
#define EDIF_TOK_CORNERTYPE 349
#define EDIF_TOK_CRITICALITY 350
#define EDIF_TOK_CURRENTMAP 351
#define EDIF_TOK_CURVE 352
#define EDIF_TOK_CYCLE 353
#define EDIF_TOK_DATAORIGIN 354
#define EDIF_TOK_DCFANINLOAD 355
#define EDIF_TOK_DCFANOUTLOAD 356
#define EDIF_TOK_DCMAXFANIN 357
#define EDIF_TOK_DCMAXFANOUT 358
#define EDIF_TOK_DELAY 359
#define EDIF_TOK_DELTA 360
#define EDIF_TOK_DERIVATION 361
#define EDIF_TOK_DESIGN 362
#define EDIF_TOK_DESIGNATOR 363
#define EDIF_TOK_DIFFERENCE 364
#define EDIF_TOK_DIRECTION 365
#define EDIF_TOK_DISPLAY 366
#define EDIF_TOK_DOMINATES 367
#define EDIF_TOK_DOT 368
#define EDIF_TOK_DURATION 369
#define EDIF_TOK_E 370
#define EDIF_TOK_EDIF 371
#define EDIF_TOK_EDIFLEVEL 372
#define EDIF_TOK_EDIFVERSION 373
#define EDIF_TOK_ENCLOSUREDISTANCE 374
#define EDIF_TOK_ENDTYPE 375
#define EDIF_TOK_ENTRY 376
#define EDIF_TOK_EVENT 377
#define EDIF_TOK_EXACTLY 378
#define EDIF_TOK_EXTERNAL 379
#define EDIF_TOK_FABRICATE 380
#define EDIF_TOK_FALSE 381
#define EDIF_TOK_FIGURE 382
#define EDIF_TOK_FIGUREAREA 383
#define EDIF_TOK_FIGUREGROUP 384
#define EDIF_TOK_FIGUREGROUPOBJECT 385
#define EDIF_TOK_FIGUREGROUPOVERRIDE 386
#define EDIF_TOK_FIGUREGROUPREF 387
#define EDIF_TOK_FIGUREPERIMETER 388
#define EDIF_TOK_FIGUREWIDTH 389
#define EDIF_TOK_FILLPATTERN 390
#define EDIF_TOK_FOLLOW 391
#define EDIF_TOK_FORBIDDENEVENT 392
#define EDIF_TOK_GLOBALPORTREF 393
#define EDIF_TOK_GREATERTHAN 394
#define EDIF_TOK_GRIDMAP 395
#define EDIF_TOK_IGNORE 396
#define EDIF_TOK_INCLUDEFIGUREGROUP 397
#define EDIF_TOK_INITIAL 398
#define EDIF_TOK_INSTANCE 399
#define EDIF_TOK_INSTANCEBACKANNOTATE 400
#define EDIF_TOK_INSTANCEGROUP 401
#define EDIF_TOK_INSTANCEMAP 402
#define EDIF_TOK_INSTANCEREF 403
#define EDIF_TOK_INTEGER 404
#define EDIF_TOK_INTEGERDISPLAY 405
#define EDIF_TOK_INTERFACE 406
#define EDIF_TOK_INTERFIGUREGROUPSPACING 407
#define EDIF_TOK_INTERSECTION 408
#define EDIF_TOK_INTRAFIGUREGROUPSPACING 409
#define EDIF_TOK_INVERSE 410
#define EDIF_TOK_ISOLATED 411
#define EDIF_TOK_JOINED 412
#define EDIF_TOK_JUSTIFY 413
#define EDIF_TOK_KEYWORDDISPLAY 414
#define EDIF_TOK_KEYWORDLEVEL 415
#define EDIF_TOK_KEYWORDMAP 416
#define EDIF_TOK_LESSTHAN 417
#define EDIF_TOK_LIBRARY 418
#define EDIF_TOK_LIBRARYREF 419
#define EDIF_TOK_LISTOFNETS 420
#define EDIF_TOK_LISTOFPORTS 421
#define EDIF_TOK_LOADDELAY 422
#define EDIF_TOK_LOGICASSIGN 423
#define EDIF_TOK_LOGICINPUT 424
#define EDIF_TOK_LOGICLIST 425
#define EDIF_TOK_LOGICMAPINPUT 426
#define EDIF_TOK_LOGICMAPOUTPUT 427
#define EDIF_TOK_LOGICONEOF 428
#define EDIF_TOK_LOGICOUTPUT 429
#define EDIF_TOK_LOGICPORT 430
#define EDIF_TOK_LOGICREF 431
#define EDIF_TOK_LOGICVALUE 432
#define EDIF_TOK_LOGICWAVEFORM 433
#define EDIF_TOK_MAINTAIN 434
#define EDIF_TOK_MATCH 435
#define EDIF_TOK_MEMBER 436
#define EDIF_TOK_MINOMAX 437
#define EDIF_TOK_MINOMAXDISPLAY 438
#define EDIF_TOK_MNM 439
#define EDIF_TOK_MULTIPLEVALUESET 440
#define EDIF_TOK_MUSTJOIN 441
#define EDIF_TOK_NAME 442
#define EDIF_TOK_NET 443
#define EDIF_TOK_NETBACKANNOTATE 444
#define EDIF_TOK_NETBUNDLE 445
#define EDIF_TOK_NETDELAY 446
#define EDIF_TOK_NETGROUP 447
#define EDIF_TOK_NETMAP 448
#define EDIF_TOK_NETREF 449
#define EDIF_TOK_NOCHANGE 450
#define EDIF_TOK_NONPERMUTABLE 451
#define EDIF_TOK_NOTALLOWED 452
#define EDIF_TOK_NOTCHSPACING 453
#define EDIF_TOK_NUMBER 454
#define EDIF_TOK_NUMBERDEFINITION 455
#define EDIF_TOK_NUMBERDISPLAY 456
#define EDIF_TOK_OFFPAGECONNECTOR 457
#define EDIF_TOK_OFFSETEVENT 458
#define EDIF_TOK_OPENSHAPE 459
#define EDIF_TOK_ORIENTATION 460
#define EDIF_TOK_ORIGIN 461
#define EDIF_TOK_OVERHANGDISTANCE 462
#define EDIF_TOK_OVERLAPDISTANCE 463
#define EDIF_TOK_OVERSIZE 464
#define EDIF_TOK_OWNER 465
#define EDIF_TOK_PAGE 466
#define EDIF_TOK_PAGESIZE 467
#define EDIF_TOK_PARAMETER 468
#define EDIF_TOK_PARAMETERASSIGN 469
#define EDIF_TOK_PARAMETERDISPLAY 470
#define EDIF_TOK_PATH 471
#define EDIF_TOK_PATHDELAY 472
#define EDIF_TOK_PATHWIDTH 473
#define EDIF_TOK_PERMUTABLE 474
#define EDIF_TOK_PHYSICALDESIGNRULE 475
#define EDIF_TOK_PLUG 476
#define EDIF_TOK_POINT 477
#define EDIF_TOK_POINTDISPLAY 478
#define EDIF_TOK_POINTLIST 479
#define EDIF_TOK_POLYGON 480
#define EDIF_TOK_PORT 481
#define EDIF_TOK_PORTBACKANNOTATE 482
#define EDIF_TOK_PORTBUNDLE 483
#define EDIF_TOK_PORTDELAY 484
#define EDIF_TOK_PORTGROUP 485
#define EDIF_TOK_PORTIMPLEMENTATION 486
#define EDIF_TOK_PORTINSTANCE 487
#define EDIF_TOK_PORTLIST 488
#define EDIF_TOK_PORTLISTALIAS 489
#define EDIF_TOK_PORTMAP 490
#define EDIF_TOK_PORTREF 491
#define EDIF_TOK_PROGRAM 492
#define EDIF_TOK_PROPERTY 493
#define EDIF_TOK_PROPERTYDISPLAY 494
#define EDIF_TOK_PROTECTIONFRAME 495
#define EDIF_TOK_PT 496
#define EDIF_TOK_RANGEVECTOR 497
#define EDIF_TOK_RECTANGLE 498
#define EDIF_TOK_RECTANGLESIZE 499
#define EDIF_TOK_RENAME 500
#define EDIF_TOK_RESOLVES 501
#define EDIF_TOK_SCALE 502
#define EDIF_TOK_SCALEX 503
#define EDIF_TOK_SCALEY 504
#define EDIF_TOK_SECTION 505
#define EDIF_TOK_SHAPE 506
#define EDIF_TOK_SIMULATE 507
#define EDIF_TOK_SIMULATIONINFO 508
#define EDIF_TOK_SINGLEVALUESET 509
#define EDIF_TOK_SITE 510
#define EDIF_TOK_SOCKET 511
#define EDIF_TOK_SOCKETSET 512
#define EDIF_TOK_STATUS 513
#define EDIF_TOK_STEADY 514
#define EDIF_TOK_STRING 515
#define EDIF_TOK_STRINGDISPLAY 516
#define EDIF_TOK_STRONG 517
#define EDIF_TOK_SYMBOL 518
#define EDIF_TOK_SYMMETRY 519
#define EDIF_TOK_TABLE 520
#define EDIF_TOK_TABLEDEFAULT 521
#define EDIF_TOK_TECHNOLOGY 522
#define EDIF_TOK_TEXTHEIGHT 523
#define EDIF_TOK_TIMEINTERVAL 524
#define EDIF_TOK_TIMESTAMP 525
#define EDIF_TOK_TIMING 526
#define EDIF_TOK_TRANSFORM 527
#define EDIF_TOK_TRANSITION 528
#define EDIF_TOK_TRIGGER 529
#define EDIF_TOK_TRUE 530
#define EDIF_TOK_UNCONSTRAINED 531
#define EDIF_TOK_UNDEFINED 532
#define EDIF_TOK_UNION 533
#define EDIF_TOK_UNIT 534
#define EDIF_TOK_UNUSED 535
#define EDIF_TOK_USERDATA 536
#define EDIF_TOK_VERSION 537
#define EDIF_TOK_VIEW 538
#define EDIF_TOK_VIEWLIST 539
#define EDIF_TOK_VIEWMAP 540
#define EDIF_TOK_VIEWREF 541
#define EDIF_TOK_VIEWTYPE 542
#define EDIF_TOK_VISIBLE 543
#define EDIF_TOK_VOLTAGEMAP 544
#define EDIF_TOK_WAVEVALUE 545
#define EDIF_TOK_WEAK 546
#define EDIF_TOK_WEAKJOINED 547
#define EDIF_TOK_WHEN 548
#define EDIF_TOK_WRITTEN 549




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE {

/* Line 1685 of yacc.c  */
#line 193 "edif.y"

	char *s;
	pair_list *pl;
	str_pair *ps;



/* Line 1685 of yacc.c  */
#line 647 "edif.h"
} YYSTYPE;
#define YYSTYPE_IS_TRIVIAL 1
#define yystype YYSTYPE					/* obsolescent; will be withdrawn */
#define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE ediflval;
