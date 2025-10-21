#include <gtest/gtest.h>
#include "ir/ir.h"
#include "ir/analysis/graph_analyzer.h"


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
