cmake_minimum_required(VERSION 3.6)
project(tigerCompiler)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g")

set(SOURCE_FILES
        lex.yy.c
        y.tab.c
        errormsg.c
        absyn.c
        parse.c
        parsetest.c
        prabsyn.c
        symbol.c
        table.c
        util.c
        semant.c
        env.c
        types.c frame.c temp.c translate.c tree.c printtree.c)

add_executable(tc ${SOURCE_FILES})