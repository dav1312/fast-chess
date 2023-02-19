#include "doctest/doctest.h"

#include "../src/board.h"

TEST_CASE("Test Zobrist Hash Startpos")
{
    Board b;

    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    CHECK(b.zobristHash() == 0x463b96181691fc9c);
    CHECK(b.getHash() == 0x463b96181691fc9c);

    b.make_move(convertUciToMove("e2e4"));
    CHECK(b.zobristHash() == 0x823c9b50fd114196);
    CHECK(b.getHash() == 0x823c9b50fd114196);

    b.make_move(convertUciToMove("d7d5"));
    CHECK(b.zobristHash() == 0x0756b94461c50fb0);
    CHECK(b.getHash() == 0x0756b94461c50fb0);

    b.make_move(convertUciToMove("e4e5"));
    CHECK(b.zobristHash() == 0x662fafb965db29d4);
    CHECK(b.getHash() == 0x662fafb965db29d4);

    b.make_move(convertUciToMove("f7f5"));
    CHECK(b.zobristHash() == 0x22a48b5a8e47ff78);
    CHECK(b.getHash() == 0x22a48b5a8e47ff78);

    b.make_move(convertUciToMove("e1e2"));
    CHECK(b.zobristHash() == 0x652a607ca3f242c1);
    CHECK(b.getHash() == 0x652a607ca3f242c1);

    b.make_move(convertUciToMove("e8f7"));
    CHECK(b.zobristHash() == 0x00fdd303c946bdd9);
    CHECK(b.getHash() == 0x00fdd303c946bdd9);
}

TEST_CASE("Test Zobrist Hash Second Position")
{
    Board b;

    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    b.make_move(convertUciToMove("a2a4"));
    b.make_move(convertUciToMove("b7b5"));
    b.make_move(convertUciToMove("h2h4"));
    b.make_move(convertUciToMove("b5b4"));
    b.make_move(convertUciToMove("c2c4"));
    CHECK(b.zobristHash() == 0x3c8123ea7b067637);

    b.make_move(convertUciToMove("b4c3"));
    b.make_move(convertUciToMove("a1a3"));
    CHECK(b.zobristHash() == 0x5c3f9b829b279560);
}