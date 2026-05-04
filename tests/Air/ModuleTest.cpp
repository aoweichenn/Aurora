#include <gtest/gtest.h>
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Function.h"

using namespace aurora;

TEST(ModuleTest, DefaultConstruction) {
    Module mod;
    EXPECT_EQ(mod.getName(), "");
    EXPECT_EQ(mod.getFunctions().size(), 0u);
    EXPECT_EQ(mod.getGlobals().size(), 0u);
}

TEST(ModuleTest, NamedConstruction) {
    Module mod("test_mod");
    EXPECT_EQ(mod.getName(), "test_mod");
}

TEST(ModuleTest, DataLayoutDefaults) {
    Module mod;
    EXPECT_TRUE(mod.getDataLayout().isLittleEndian());
    EXPECT_EQ(mod.getDataLayout().getPointerSize(), 64u);
}

TEST(ModuleTest, SetDataLayout) {
    Module mod;
    mod.getDataLayout().setLittleEndian(false);
    EXPECT_FALSE(mod.getDataLayout().isLittleEndian());
    mod.getDataLayout().setLittleEndian(true);
    EXPECT_TRUE(mod.getDataLayout().isLittleEndian());
    mod.getDataLayout().setPointerSize(32);
    EXPECT_EQ(mod.getDataLayout().getPointerSize(), 32u);
    mod.getDataLayout().setPointerSize(64);
    EXPECT_EQ(mod.getDataLayout().getPointerSize(), 64u);
}

TEST(ModuleTest, CreateFunction) {
    Module mod("m");
    SmallVector<Type*, 8> params;
    params.push_back(Type::getInt32Ty());
    auto* fnTy = new FunctionType(Type::getInt32Ty(), params);
    auto* fn = mod.createFunction(fnTy, "add");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getName(), "add");
    EXPECT_EQ(mod.getFunctions().size(), 1u);
}

TEST(ModuleTest, CreateMultipleFunctions) {
    Module mod("m");
    SmallVector<Type*, 8> params;
    params.push_back(Type::getInt64Ty());
    auto* fnTy = new FunctionType(Type::getInt64Ty(), params);

    auto* fn1 = mod.createFunction(fnTy, "f1");
    auto* fn2 = mod.createFunction(fnTy, "f2");
    auto* fn3 = mod.createFunction(fnTy, "f3");

    EXPECT_NE(fn1, nullptr);
    EXPECT_NE(fn2, nullptr);
    EXPECT_NE(fn3, nullptr);
    EXPECT_EQ(mod.getFunctions().size(), 3u);
    EXPECT_EQ(mod.getFunctions()[0]->getName(), "f1");
    EXPECT_EQ(mod.getFunctions()[1]->getName(), "f2");
    EXPECT_EQ(mod.getFunctions()[2]->getName(), "f3");
}

TEST(ModuleTest, CreateGlobal) {
    Module mod("m");
    auto* gv = mod.createGlobal(Type::getInt32Ty(), "x");
    ASSERT_NE(gv, nullptr);
    EXPECT_EQ(gv->getName(), "x");
    EXPECT_EQ(mod.getGlobals().size(), 1u);
}

TEST(ModuleTest, CreateMultipleGlobals) {
    Module mod("m");
    auto* gv1 = mod.createGlobal(Type::getInt32Ty(), "a");
    auto* gv2 = mod.createGlobal(Type::getPointerTy(Type::getInt64Ty()), "b");
    EXPECT_EQ(mod.getGlobals().size(), 2u);
    EXPECT_EQ(gv1->getName(), "a");
    EXPECT_EQ(gv2->getName(), "b");
}

TEST(ModuleTest, FunctionsAndGlobalsMixed) {
    Module mod("mixed");
    SmallVector<Type*, 8> params;
    auto* fnTy = new FunctionType(Type::getVoidTy(), params);

    (void)mod.createFunction(fnTy, "main");
    (void)mod.createGlobal(Type::getInt32Ty(), "counter");
    (void)mod.createFunction(fnTy, "init");

    EXPECT_EQ(mod.getFunctions().size(), 2u);
    EXPECT_EQ(mod.getGlobals().size(), 1u);
}

TEST(ModuleTest, ConstDataLayout) {
    const Module mod("ro");
    EXPECT_TRUE(mod.getDataLayout().isLittleEndian());
    EXPECT_EQ(mod.getDataLayout().getPointerSize(), 64u);
}
