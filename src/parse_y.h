/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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

#ifndef YY_YY_PARSE_Y_TAB_H_INCLUDED
# define YY_YY_PARSE_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    FLOATING = 258,
    INTEGER = 259,
    CHAR_CONST = 260,
    STRING = 261,
    T_FILEVERSION = 262,
    T_PCB = 263,
    T_LAYER = 264,
    T_VIA = 265,
    T_RAT = 266,
    T_LINE = 267,
    T_ARC = 268,
    T_RECTANGLE = 269,
    T_TEXT = 270,
    T_ELEMENTLINE = 271,
    T_ELEMENT = 272,
    T_PIN = 273,
    T_PAD = 274,
    T_GRID = 275,
    T_FLAGS = 276,
    T_SYMBOL = 277,
    T_SYMBOLLINE = 278,
    T_CURSOR = 279,
    T_ELEMENTARC = 280,
    T_MARK = 281,
    T_GROUPS = 282,
    T_STYLES = 283,
    T_POLYGON = 284,
    T_POLYGON_HOLE = 285,
    T_NETLIST = 286,
    T_NET = 287,
    T_CONN = 288,
    T_AREA = 289,
    T_THERMAL = 290,
    T_DRC = 291,
    T_ATTRIBUTE = 292,
    T_UMIL = 293,
    T_CMIL = 294,
    T_MIL = 295,
    T_IN = 296,
    T_NM = 297,
    T_UM = 298,
    T_MM = 299,
    T_M = 300,
    T_KM = 301
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 109 "parse_y.y" /* yacc.c:1909  */

	int		integer;
	double		number;
	char		*string;
	FlagType	flagtype;
	PLMeasure	measure;

#line 109 "parse_y.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSE_Y_TAB_H_INCLUDED  */
