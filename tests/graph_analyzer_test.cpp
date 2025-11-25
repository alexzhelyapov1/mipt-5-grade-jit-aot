#include <gtest/gtest.h>
#include "ir/ir.h"
#include "ir/analysis/graph_analyzer.h"
#include "ir/analysis/loop_analyzer.h"


TEST(GraphAnalyzer, RPONumbering) {
    Graph graph;
    IRBuilder builder(&graph);

    // A -> B -> C
    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    builder.CreateJump(C);

    builder.SetInsertPoint(C);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    GraphAnalyzer analyzer(&graph);
    analyzer.ComputeRPO();

    const auto& rpo = analyzer.GetReversePostOrder();
    const auto& rpo_numbers = analyzer.GetRPONumbers();

    ASSERT_EQ(rpo.size(), 3);
    EXPECT_EQ(rpo[0], A);
    EXPECT_EQ(rpo[1], B);
    EXPECT_EQ(rpo[2], C);

    EXPECT_EQ(rpo_numbers.at(A), 0);
    EXPECT_EQ(rpo_numbers.at(B), 1);
    EXPECT_EQ(rpo_numbers.at(C), 2);
}


TEST(GraphAnalyzer, LoopDetection) {
    Graph graph;
    IRBuilder builder(&graph);

    // B->C->B
    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond, C, A);

    builder.SetInsertPoint(C);
    builder.CreateJump(B);

    GraphAnalyzer analyzer(&graph);
    analyzer.BuildDominatorTree();

    const auto& rpo = analyzer.GetReversePostOrder();
    EXPECT_EQ(rpo.size(), 3);
    EXPECT_EQ(rpo[0], A);
    EXPECT_EQ(analyzer.GetImmediateDominator(C), B);
}


TEST(GraphAnalyzer, LoopHeadsAndBackEdges) {
    Graph graph;
    IRBuilder builder(&graph);

    // A -> B -> C -> B
    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond, C, A);

    builder.SetInsertPoint(C);
    builder.CreateJump(B);

    GraphAnalyzer analyzer(&graph);
    analyzer.BuildDominatorTree();

    const auto& rpo = analyzer.GetReversePostOrder();
    EXPECT_EQ(rpo.size(), 3);
    EXPECT_EQ(rpo[0], A);
    EXPECT_EQ(analyzer.GetImmediateDominator(C), B);
}


// 1
TEST(GraphAnalyzer, Example1) {
    Graph graph;
    IRBuilder builder(&graph);

    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();
    BasicBlock* D = graph.CreateBasicBlock();
    BasicBlock* E = graph.CreateBasicBlock();
    BasicBlock* F = graph.CreateBasicBlock();
    BasicBlock* G = graph.CreateBasicBlock();

    // A -> B
    // B -> C, F
    // C -> D
    // F -> E, G
    // E -> D
    // G -> D

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond, C, F);

    builder.SetInsertPoint(C);
    builder.CreateJump(D);

    builder.SetInsertPoint(F);
    cond = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond, E, G);

    builder.SetInsertPoint(E);
    builder.CreateJump(D);

    builder.SetInsertPoint(G);
    builder.CreateJump(D);

    builder.SetInsertPoint(D);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    GraphAnalyzer analyzer(&graph);
    analyzer.BuildDominatorTree();

    const auto& rpo = analyzer.GetReversePostOrder();
    const auto& rpo_numbers = analyzer.GetRPONumbers();

    // A, B, F, G, E, C, D
    ASSERT_EQ(rpo.size(), 7);
    EXPECT_EQ(rpo[0], A);
    EXPECT_EQ(rpo[1], B);
    EXPECT_EQ(rpo[2], F);
    EXPECT_EQ(rpo[3], G);
    EXPECT_EQ(rpo[4], E);
    EXPECT_EQ(rpo[5], C);
    EXPECT_EQ(rpo[6], D);

    EXPECT_EQ(rpo_numbers.at(A), 0);
    EXPECT_EQ(rpo_numbers.at(B), 1);
    EXPECT_EQ(rpo_numbers.at(C), 5);
    EXPECT_EQ(rpo_numbers.at(D), 6);
    EXPECT_EQ(rpo_numbers.at(E), 4);
    EXPECT_EQ(rpo_numbers.at(F), 2);
    EXPECT_EQ(rpo_numbers.at(G), 3);

    EXPECT_EQ(analyzer.GetImmediateDominator(A), nullptr);
    EXPECT_EQ(analyzer.GetImmediateDominator(B), A);
    EXPECT_EQ(analyzer.GetImmediateDominator(C), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(F), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(E), F);
    EXPECT_EQ(analyzer.GetImmediateDominator(G), F);
    EXPECT_EQ(analyzer.GetImmediateDominator(D), B);
}


// 2
TEST(GraphAnalyzer, Example2) {
    Graph graph;
    IRBuilder builder(&graph);

    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();
    BasicBlock* D = graph.CreateBasicBlock();
    BasicBlock* E = graph.CreateBasicBlock();
    BasicBlock* F = graph.CreateBasicBlock();
    BasicBlock* G = graph.CreateBasicBlock();
    BasicBlock* H = graph.CreateBasicBlock();
    BasicBlock* I = graph.CreateBasicBlock();
    BasicBlock* J = graph.CreateBasicBlock();
    BasicBlock* K = graph.CreateBasicBlock();

    // A -> B
    // B -> C, J
    // C -> D
    // D -> C, E
    // E -> F
    // F -> E, G
    // G -> H, I
    // H -> B
    // I -> K
    // J -> C

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, C, J);

    builder.SetInsertPoint(C);
    builder.CreateJump(D);

    builder.SetInsertPoint(D);
    auto* cond2 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond2, C, E);

    builder.SetInsertPoint(E);
    builder.CreateJump(F);

    builder.SetInsertPoint(F);
    auto* cond3 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond3, E, G);

    builder.SetInsertPoint(G);
    auto* cond4 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond4, H, I);

    builder.SetInsertPoint(H);
    builder.CreateJump(B);

    builder.SetInsertPoint(I);
    builder.CreateJump(K);

    builder.SetInsertPoint(J);
    builder.CreateJump(C);

    builder.SetInsertPoint(K);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    GraphAnalyzer analyzer(&graph);
    analyzer.BuildDominatorTree();

    const auto& rpo = analyzer.GetReversePostOrder();
    const auto& rpo_numbers = analyzer.GetRPONumbers();

    EXPECT_EQ(rpo.size(), 11);
    EXPECT_EQ(rpo[0], A);

    for (size_t i = 0; i < rpo.size(); ++i) {
        EXPECT_EQ(rpo_numbers.at(rpo[i]), i);
    }

    EXPECT_EQ(analyzer.GetImmediateDominator(A), nullptr);
    EXPECT_EQ(analyzer.GetImmediateDominator(B), A);
    EXPECT_EQ(analyzer.GetImmediateDominator(C), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(D), C);
    EXPECT_EQ(analyzer.GetImmediateDominator(E), D);
    EXPECT_EQ(analyzer.GetImmediateDominator(F), E);
    EXPECT_EQ(analyzer.GetImmediateDominator(G), F);
    EXPECT_EQ(analyzer.GetImmediateDominator(H), G);
    EXPECT_EQ(analyzer.GetImmediateDominator(I), G);
    EXPECT_EQ(analyzer.GetImmediateDominator(J), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(K), I);

    std::unordered_set<BasicBlock*> all_blocks = {A, B, C, D, E, F, G, H, I, J, K};
    std::unordered_set<BasicBlock*> rpo_set(rpo.begin(), rpo.end());
    EXPECT_EQ(rpo_set, all_blocks);

    size_t k_pos = rpo_numbers.at(K);
    EXPECT_GT(k_pos, rpo_numbers.at(A));
    EXPECT_GT(k_pos, rpo_numbers.at(B));
}


// 3
TEST(GraphAnalyzer, Example3) {
    Graph graph;
    IRBuilder builder(&graph);

    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();
    BasicBlock* D = graph.CreateBasicBlock();
    BasicBlock* E = graph.CreateBasicBlock();
    BasicBlock* F = graph.CreateBasicBlock();
    BasicBlock* G = graph.CreateBasicBlock();
    BasicBlock* H = graph.CreateBasicBlock();
    BasicBlock* I = graph.CreateBasicBlock();

    // Строим граф согласно схеме:
    // A -> B
    // B -> C, E
    // C -> D
    // D -> G
    // E -> D, F
    // F -> B, H
    // G -> C, I
    // H -> G, I

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, C, E);

    builder.SetInsertPoint(C);
    builder.CreateJump(D);

    builder.SetInsertPoint(D);
    builder.CreateJump(G);

    builder.SetInsertPoint(E);
    auto* cond2 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond2, D, F);

    builder.SetInsertPoint(F);
    auto* cond3 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond3, B, H);

    builder.SetInsertPoint(G);
    auto* cond4 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond4, C, I);

    builder.SetInsertPoint(H);
    auto* cond5 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond5, G, I);

    builder.SetInsertPoint(I);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    GraphAnalyzer analyzer(&graph);
    analyzer.BuildDominatorTree();

    const auto& rpo = analyzer.GetReversePostOrder();
    const auto& rpo_numbers = analyzer.GetRPONumbers();

    EXPECT_EQ(rpo.size(), 9);
    EXPECT_EQ(rpo[0], A);
    for (size_t i = 0; i < rpo.size(); ++i) {
        EXPECT_EQ(rpo_numbers.at(rpo[i]), i);
    }

    EXPECT_EQ(analyzer.GetImmediateDominator(A), nullptr);
    EXPECT_EQ(analyzer.GetImmediateDominator(B), A);
    EXPECT_EQ(analyzer.GetImmediateDominator(C), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(D), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(E), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(F), E);
    EXPECT_EQ(analyzer.GetImmediateDominator(G), B);
    EXPECT_EQ(analyzer.GetImmediateDominator(H), F);
    EXPECT_EQ(analyzer.GetImmediateDominator(I), B);
}

TEST(LoopAnalyzer, SimpleLoop) {
    Graph graph;
    IRBuilder builder(&graph);

    // a -> b
    // b -> c, d
    // d -> e
    // e -> b
    BasicBlock* a = graph.CreateBasicBlock();
    BasicBlock* b = graph.CreateBasicBlock();
    BasicBlock* c = graph.CreateBasicBlock();
    BasicBlock* d = graph.CreateBasicBlock();
    BasicBlock* e = graph.CreateBasicBlock();

    builder.SetInsertPoint(a);
    builder.CreateJump(b);

    builder.SetInsertPoint(b);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, c, d);

    builder.SetInsertPoint(c);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    builder.SetInsertPoint(d);
    builder.CreateJump(e);

    builder.SetInsertPoint(e);
    builder.CreateJump(b);

    LoopAnalyzer analyzer(&graph);
    analyzer.Analyze();

    const auto& loops = analyzer.GetLoops();
    ASSERT_EQ(loops.size(), 1);

    Loop* loop = loops[0];

    EXPECT_EQ(loop->GetHeader(), b);

    const auto& loop_blocks = loop->GetBlocks();
    EXPECT_EQ(loop_blocks.size(), 3);
    EXPECT_NE(std::find(loop_blocks.begin(), loop_blocks.end(), b), loop_blocks.end());
    EXPECT_NE(std::find(loop_blocks.begin(), loop_blocks.end(), d), loop_blocks.end());
    EXPECT_NE(std::find(loop_blocks.begin(), loop_blocks.end(), e), loop_blocks.end());

    const auto& back_edges = loop->GetBackEdges();
    ASSERT_EQ(back_edges.size(), 1);
    EXPECT_EQ(back_edges[0], e);

    EXPECT_TRUE(loop->IsReducible());
    EXPECT_TRUE(loop->IsCountable());

    EXPECT_EQ(analyzer.GetLoopForBlock(b), loop);
    EXPECT_EQ(analyzer.GetLoopForBlock(d), loop);
    EXPECT_EQ(analyzer.GetLoopForBlock(e), loop);
    EXPECT_EQ(analyzer.GetLoopForBlock(a), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(c), nullptr);

    EXPECT_TRUE(analyzer.IsLoopHeader(b));
    EXPECT_FALSE(analyzer.IsLoopHeader(a));
    EXPECT_FALSE(analyzer.IsLoopHeader(c));
    EXPECT_FALSE(analyzer.IsLoopHeader(d));
    EXPECT_FALSE(analyzer.IsLoopHeader(e));
}

TEST(LoopAnalyzer, NestedStructureLoop) {
    Graph graph;
    IRBuilder builder(&graph);

    // a -> b
    // b -> c
    // c -> d, e
    // d -> e, f
    // f -> b
    BasicBlock* a = graph.CreateBasicBlock();
    BasicBlock* b = graph.CreateBasicBlock();
    BasicBlock* c = graph.CreateBasicBlock();
    BasicBlock* d = graph.CreateBasicBlock();
    BasicBlock* e = graph.CreateBasicBlock();
    BasicBlock* f = graph.CreateBasicBlock();

    builder.SetInsertPoint(a);
    builder.CreateJump(b);

    builder.SetInsertPoint(b);
    builder.CreateJump(c);

    builder.SetInsertPoint(c);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, d, e);

    builder.SetInsertPoint(d);
    auto* cond2 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond2, e, f);

    builder.SetInsertPoint(e);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    builder.SetInsertPoint(f);
    builder.CreateJump(b);

    LoopAnalyzer analyzer(&graph);
    analyzer.Analyze();

    const auto& loops = analyzer.GetLoops();
    ASSERT_EQ(loops.size(), 1);

    Loop* loop = loops[0];

    EXPECT_EQ(loop->GetHeader(), b);

    const auto& loop_blocks = loop->GetBlocks();
    EXPECT_EQ(loop_blocks.size(), 4);
    EXPECT_NE(std::find(loop_blocks.begin(), loop_blocks.end(), b), loop_blocks.end());
    EXPECT_NE(std::find(loop_blocks.begin(), loop_blocks.end(), c), loop_blocks.end());
    EXPECT_NE(std::find(loop_blocks.begin(), loop_blocks.end(), d), loop_blocks.end());
    EXPECT_NE(std::find(loop_blocks.begin(), loop_blocks.end(), f), loop_blocks.end());

    const auto& back_edges = loop->GetBackEdges();
    ASSERT_EQ(back_edges.size(), 1);
    EXPECT_EQ(back_edges[0], f);

    EXPECT_TRUE(loop->IsReducible());
    EXPECT_TRUE(loop->IsCountable());

    EXPECT_EQ(analyzer.GetLoopForBlock(b), loop);
    EXPECT_EQ(analyzer.GetLoopForBlock(c), loop);
    EXPECT_EQ(analyzer.GetLoopForBlock(d), loop);
    EXPECT_EQ(analyzer.GetLoopForBlock(f), loop);
    EXPECT_EQ(analyzer.GetLoopForBlock(a), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(e), nullptr);
}

TEST(LoopAnalyzer, ComplexLoopStructure) {
    Graph graph;
    IRBuilder builder(&graph);

    // a -> b
    // b -> c, d
    // c -> e, f
    // d -> f
    // f -> g
    // g -> h, b
    // h -> a
    BasicBlock* a = graph.CreateBasicBlock();
    BasicBlock* b = graph.CreateBasicBlock();
    BasicBlock* c = graph.CreateBasicBlock();
    BasicBlock* d = graph.CreateBasicBlock();
    BasicBlock* e = graph.CreateBasicBlock();
    BasicBlock* f = graph.CreateBasicBlock();
    BasicBlock* g = graph.CreateBasicBlock();
    BasicBlock* h = graph.CreateBasicBlock();

    builder.SetInsertPoint(a);
    builder.CreateJump(b);

    builder.SetInsertPoint(b);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, c, d);

    builder.SetInsertPoint(c);
    auto* cond2 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond2, e, f);

    builder.SetInsertPoint(d);
    builder.CreateJump(f);

    builder.SetInsertPoint(e);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    builder.SetInsertPoint(f);
    builder.CreateJump(g);

    builder.SetInsertPoint(g);
    auto* cond3 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond3, h, b);

    builder.SetInsertPoint(h);
    builder.CreateJump(a);

    LoopAnalyzer analyzer(&graph);
    analyzer.Analyze();

    const auto& loops = analyzer.GetLoops();
    ASSERT_EQ(loops.size(), 2);

    Loop* loop_a = nullptr;
    Loop* loop_b = nullptr;

    for (Loop* loop : loops) {
        if (loop->GetHeader() == a) {
            loop_a = loop;
        } else if (loop->GetHeader() == b) {
            loop_b = loop;
        }
    }

    ASSERT_NE(loop_a, nullptr);
    ASSERT_NE(loop_b, nullptr);

    const auto& loop_a_blocks = loop_a->GetBlocks();
    EXPECT_EQ(loop_a_blocks.size(), 7); // a, b, c, d, f, g, h
    EXPECT_NE(std::find(loop_a_blocks.begin(), loop_a_blocks.end(), a), loop_a_blocks.end());
    EXPECT_NE(std::find(loop_a_blocks.begin(), loop_a_blocks.end(), b), loop_a_blocks.end());
    EXPECT_NE(std::find(loop_a_blocks.begin(), loop_a_blocks.end(), c), loop_a_blocks.end());
    EXPECT_NE(std::find(loop_a_blocks.begin(), loop_a_blocks.end(), d), loop_a_blocks.end());
    EXPECT_NE(std::find(loop_a_blocks.begin(), loop_a_blocks.end(), f), loop_a_blocks.end());
    EXPECT_NE(std::find(loop_a_blocks.begin(), loop_a_blocks.end(), g), loop_a_blocks.end());
    EXPECT_NE(std::find(loop_a_blocks.begin(), loop_a_blocks.end(), h), loop_a_blocks.end());

    const auto& loop_a_back_edges = loop_a->GetBackEdges();
    ASSERT_EQ(loop_a_back_edges.size(), 1);
    EXPECT_EQ(loop_a_back_edges[0], h);

    const auto& loop_b_blocks = loop_b->GetBlocks();
    EXPECT_EQ(loop_b_blocks.size(), 5); // b, c, d, f, g
    EXPECT_NE(std::find(loop_b_blocks.begin(), loop_b_blocks.end(), b), loop_b_blocks.end());
    EXPECT_NE(std::find(loop_b_blocks.begin(), loop_b_blocks.end(), c), loop_b_blocks.end());
    EXPECT_NE(std::find(loop_b_blocks.begin(), loop_b_blocks.end(), d), loop_b_blocks.end());
    EXPECT_NE(std::find(loop_b_blocks.begin(), loop_b_blocks.end(), f), loop_b_blocks.end());
    EXPECT_NE(std::find(loop_b_blocks.begin(), loop_b_blocks.end(), g), loop_b_blocks.end());

    const auto& loop_b_back_edges = loop_b->GetBackEdges();
    ASSERT_EQ(loop_b_back_edges.size(), 1);
    EXPECT_EQ(loop_b_back_edges[0], g);

    EXPECT_TRUE(loop_a->IsReducible());
    EXPECT_TRUE(loop_b->IsReducible());

    EXPECT_EQ(loop_b->GetOuterLoop(), loop_a);
    EXPECT_EQ(loop_a->GetInnerLoops().size(), 1);
    EXPECT_EQ(loop_a->GetInnerLoops()[0], loop_b);

    EXPECT_EQ(analyzer.GetLoopForBlock(a), loop_a);
    EXPECT_EQ(analyzer.GetLoopForBlock(b), loop_b);
    EXPECT_EQ(analyzer.GetLoopForBlock(c), loop_b);
    EXPECT_EQ(analyzer.GetLoopForBlock(d), loop_b);
    EXPECT_EQ(analyzer.GetLoopForBlock(f), loop_b);
    EXPECT_EQ(analyzer.GetLoopForBlock(g), loop_b);
    EXPECT_EQ(analyzer.GetLoopForBlock(h), loop_a);
    EXPECT_EQ(analyzer.GetLoopForBlock(e), nullptr);

    EXPECT_TRUE(analyzer.IsLoopHeader(a));
    EXPECT_TRUE(analyzer.IsLoopHeader(b));
    EXPECT_FALSE(analyzer.IsLoopHeader(c));
    EXPECT_FALSE(analyzer.IsLoopHeader(d));
    EXPECT_FALSE(analyzer.IsLoopHeader(e));
    EXPECT_FALSE(analyzer.IsLoopHeader(f));
    EXPECT_FALSE(analyzer.IsLoopHeader(g));
    EXPECT_FALSE(analyzer.IsLoopHeader(h));
}


TEST(LoopAnalyzer, NoLoopsGraph) {
    Graph graph;
    IRBuilder builder(&graph);

    // A -> B
    // B -> C, F
    // C -> D
    // F -> E, G
    // E -> D
    // G -> D
    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();
    BasicBlock* D = graph.CreateBasicBlock();
    BasicBlock* E = graph.CreateBasicBlock();
    BasicBlock* F = graph.CreateBasicBlock();
    BasicBlock* G = graph.CreateBasicBlock();

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, C, F);

    builder.SetInsertPoint(C);
    builder.CreateJump(D);

    builder.SetInsertPoint(F);
    auto* cond2 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond2, E, G);

    builder.SetInsertPoint(E);
    builder.CreateJump(D);

    builder.SetInsertPoint(G);
    builder.CreateJump(D);

    builder.SetInsertPoint(D);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    LoopAnalyzer analyzer(&graph);
    analyzer.Analyze();

    EXPECT_EQ(analyzer.GetLoops().size(), 0);

    EXPECT_FALSE(analyzer.IsLoopHeader(A));
    EXPECT_FALSE(analyzer.IsLoopHeader(B));
    EXPECT_FALSE(analyzer.IsLoopHeader(C));
    EXPECT_FALSE(analyzer.IsLoopHeader(D));
    EXPECT_FALSE(analyzer.IsLoopHeader(E));
    EXPECT_FALSE(analyzer.IsLoopHeader(F));
    EXPECT_FALSE(analyzer.IsLoopHeader(G));

    EXPECT_EQ(analyzer.GetLoopForBlock(A), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(B), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(C), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(D), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(E), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(F), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(G), nullptr);
}

TEST(LoopAnalyzer, MultipleNestedLoops) {
    Graph graph;
    IRBuilder builder(&graph);

    // A -> B
    // B -> C, J
    // C -> D
    // D -> C, E
    // E -> F
    // F -> E, G
    // G -> H, I
    // H -> B
    // I -> K
    // J -> C
    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();
    BasicBlock* D = graph.CreateBasicBlock();
    BasicBlock* E = graph.CreateBasicBlock();
    BasicBlock* F = graph.CreateBasicBlock();
    BasicBlock* G = graph.CreateBasicBlock();
    BasicBlock* H = graph.CreateBasicBlock();
    BasicBlock* I = graph.CreateBasicBlock();
    BasicBlock* J = graph.CreateBasicBlock();
    BasicBlock* K = graph.CreateBasicBlock();

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, C, J);

    builder.SetInsertPoint(C);
    builder.CreateJump(D);

    builder.SetInsertPoint(D);
    auto* cond2 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond2, C, E);

    builder.SetInsertPoint(E);
    builder.CreateJump(F);

    builder.SetInsertPoint(F);
    auto* cond3 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond3, E, G);

    builder.SetInsertPoint(G);
    auto* cond4 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond4, H, I);

    builder.SetInsertPoint(H);
    builder.CreateJump(B);

    builder.SetInsertPoint(I);
    builder.CreateJump(K);

    builder.SetInsertPoint(J);
    builder.CreateJump(C);

    builder.SetInsertPoint(K);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    LoopAnalyzer analyzer(&graph);
    analyzer.Analyze();

    const auto& loops = analyzer.GetLoops();
    ASSERT_EQ(loops.size(), 3);

    Loop* loop_B = nullptr;
    Loop* loop_C = nullptr;
    Loop* loop_E = nullptr;

    for (Loop* loop : loops) {
        if (loop->GetHeader() == B) {
            loop_B = loop;
        }
        if (loop->GetHeader() == C) {
            loop_C = loop;
        }
        if (loop->GetHeader() == E) {
            loop_E = loop;
        }
    }

    ASSERT_NE(loop_B, nullptr);
    ASSERT_NE(loop_C, nullptr);
    ASSERT_NE(loop_E, nullptr);

    EXPECT_TRUE(loop_C->ContainsBlock(C));
    EXPECT_TRUE(loop_C->ContainsBlock(D));
    EXPECT_EQ(loop_C->GetBackEdges().size(), 1);
    EXPECT_EQ(loop_C->GetBackEdges()[0], D);

    EXPECT_TRUE(loop_E->ContainsBlock(E));
    EXPECT_TRUE(loop_E->ContainsBlock(F));
    EXPECT_EQ(loop_E->GetBackEdges().size(), 1);
    EXPECT_EQ(loop_E->GetBackEdges()[0], F);

    EXPECT_TRUE(loop_B->ContainsBlock(B));
    EXPECT_TRUE(loop_B->ContainsBlock(C));
    EXPECT_TRUE(loop_B->ContainsBlock(D));
    EXPECT_TRUE(loop_B->ContainsBlock(E));
    EXPECT_TRUE(loop_B->ContainsBlock(F));
    EXPECT_TRUE(loop_B->ContainsBlock(G));
    EXPECT_TRUE(loop_B->ContainsBlock(H));
    EXPECT_TRUE(loop_B->ContainsBlock(J));
    EXPECT_EQ(loop_B->GetBackEdges().size(), 1);
    EXPECT_EQ(loop_B->GetBackEdges()[0], H);

    EXPECT_EQ(loop_C->GetOuterLoop(), loop_B);
    EXPECT_EQ(loop_E->GetOuterLoop(), loop_B);
    EXPECT_TRUE(loop_B->ContainsLoop(loop_C));
    EXPECT_TRUE(loop_B->ContainsLoop(loop_E));

    EXPECT_EQ(analyzer.GetLoopForBlock(A), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(B), loop_B);
    EXPECT_EQ(analyzer.GetLoopForBlock(J), loop_B);
    EXPECT_EQ(analyzer.GetLoopForBlock(I), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(K), nullptr);

    EXPECT_TRUE(analyzer.IsLoopHeader(B));
    EXPECT_TRUE(analyzer.IsLoopHeader(C));
    EXPECT_TRUE(analyzer.IsLoopHeader(E));
    EXPECT_FALSE(analyzer.IsLoopHeader(A));
    EXPECT_FALSE(analyzer.IsLoopHeader(D));
    EXPECT_FALSE(analyzer.IsLoopHeader(F));
    EXPECT_FALSE(analyzer.IsLoopHeader(G));
    EXPECT_FALSE(analyzer.IsLoopHeader(H));
    EXPECT_FALSE(analyzer.IsLoopHeader(I));
    EXPECT_FALSE(analyzer.IsLoopHeader(J));
    EXPECT_FALSE(analyzer.IsLoopHeader(K));
}

TEST(LoopAnalyzer, TwoNestedLoopsComplex) {
    Graph graph;
    IRBuilder builder(&graph);

    // A -> B
    // B -> C, E
    // C -> D
    // D -> G
    // E -> D, F
    // F -> B, H
    // G -> C, I
    // H -> G, I
    BasicBlock* A = graph.CreateBasicBlock();
    BasicBlock* B = graph.CreateBasicBlock();
    BasicBlock* C = graph.CreateBasicBlock();
    BasicBlock* D = graph.CreateBasicBlock();
    BasicBlock* E = graph.CreateBasicBlock();
    BasicBlock* F = graph.CreateBasicBlock();
    BasicBlock* G = graph.CreateBasicBlock();
    BasicBlock* H = graph.CreateBasicBlock();
    BasicBlock* I = graph.CreateBasicBlock();

    builder.SetInsertPoint(A);
    builder.CreateJump(B);

    builder.SetInsertPoint(B);
    auto* cond1 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond1, C, E);

    builder.SetInsertPoint(C);
    builder.CreateJump(D);

    builder.SetInsertPoint(D);
    builder.CreateJump(G);

    builder.SetInsertPoint(E);
    auto* cond2 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond2, D, F);

    builder.SetInsertPoint(F);
    auto* cond3 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond3, B, H);

    builder.SetInsertPoint(G);
    auto* cond4 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond4, C, I);

    builder.SetInsertPoint(H);
    auto* cond5 = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond5, G, I);

    builder.SetInsertPoint(I);
    builder.CreateRet(builder.CreateConstant(Type::U32, 0));

    LoopAnalyzer analyzer(&graph);
    analyzer.Analyze();

    const auto& loops = analyzer.GetLoops();
    ASSERT_EQ(loops.size(), 2);

    Loop* loop_B = nullptr;
    Loop* loop_C = nullptr;

    for (Loop* loop : loops) {
        if (loop->GetHeader() == B) loop_B = loop;
        if (loop->GetHeader() == C) loop_C = loop;
    }

    ASSERT_NE(loop_B, nullptr);
    ASSERT_NE(loop_C, nullptr);

    EXPECT_TRUE(loop_B->ContainsBlock(B));
    EXPECT_TRUE(loop_B->ContainsBlock(E));
    EXPECT_TRUE(loop_B->ContainsBlock(F));
    EXPECT_EQ(loop_B->GetBackEdges().size(), 1);
    EXPECT_EQ(loop_B->GetBackEdges()[0], F);

    EXPECT_TRUE(loop_C->ContainsBlock(C));
    EXPECT_TRUE(loop_C->ContainsBlock(D));
    EXPECT_TRUE(loop_C->ContainsBlock(G));
    EXPECT_EQ(loop_C->GetBackEdges().size(), 1);
    EXPECT_EQ(loop_C->GetBackEdges()[0], G);

    EXPECT_EQ(loop_C->GetOuterLoop(), nullptr);
    EXPECT_EQ(loop_B->GetOuterLoop(), nullptr);

    EXPECT_EQ(analyzer.GetLoopForBlock(A), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(B), loop_B);
    EXPECT_EQ(analyzer.GetLoopForBlock(C), loop_C);
    EXPECT_EQ(analyzer.GetLoopForBlock(D), loop_C);
    EXPECT_EQ(analyzer.GetLoopForBlock(E), loop_B);
    EXPECT_EQ(analyzer.GetLoopForBlock(F), loop_B);
    EXPECT_EQ(analyzer.GetLoopForBlock(G), loop_C);
    EXPECT_EQ(analyzer.GetLoopForBlock(H), nullptr);
    EXPECT_EQ(analyzer.GetLoopForBlock(I), nullptr);

    EXPECT_TRUE(analyzer.IsLoopHeader(B));
    EXPECT_TRUE(analyzer.IsLoopHeader(C));
    EXPECT_FALSE(analyzer.IsLoopHeader(A));
    EXPECT_FALSE(analyzer.IsLoopHeader(D));
    EXPECT_FALSE(analyzer.IsLoopHeader(E));
    EXPECT_FALSE(analyzer.IsLoopHeader(F));
    EXPECT_FALSE(analyzer.IsLoopHeader(G));
    EXPECT_FALSE(analyzer.IsLoopHeader(H));
    EXPECT_FALSE(analyzer.IsLoopHeader(I));
}
