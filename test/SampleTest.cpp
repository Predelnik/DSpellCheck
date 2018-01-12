#include "catch.hpp"
#include "SpellChecker.h"
#include "MockSpeller.h"
#include "MockEditorInterface.h"
#include "Settings.h"

TEST_CASE ("Simple")
{
    MockSpeller speller;
    Settings settings;
    MockEditorInterface editor;
}