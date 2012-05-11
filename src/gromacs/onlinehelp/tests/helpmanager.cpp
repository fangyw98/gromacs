/*
 *
 *                This source code is part of
 *
 *                 G   R   O   M   A   C   S
 *
 *          GROningen MAchine for Chemical Simulations
 *
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2009, The GROMACS development team,
 * check out http://www.gromacs.org for more information.

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 *
 * For more info, check our website at http://www.gromacs.org
 */
/*! \internal \file
 * \brief
 * Tests for help topic management and help topic formatting.
 *
 * \author Teemu Murtola <teemu.murtola@cbr.su.se>
 * \ingroup module_onlinehelp
 */
#include "gromacs/onlinehelp/helpmanager.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "gromacs/onlinehelp/helptopic.h"
#include "gromacs/utility/exceptions.h"
#include "gromacs/utility/file.h"

#include "testutils/datapath.h"
#include "testutils/mock_helptopic.h"
#include "testutils/stringtest.h"

namespace
{

using gmx::test::MockHelpTopic;

class HelpTestBase : public gmx::test::StringTestBase
{
    public:
        HelpTestBase();

        gmx::test::TestTemporaryFileManager tempFiles_;
        MockHelpTopic           rootTopic_;
        gmx::HelpManager        manager_;
};

HelpTestBase::HelpTestBase()
    : rootTopic_("", NULL, "Root topic text"),
      manager_(rootTopic_)
{
}

/********************************************************************
 * Tests for HelpManager
 */

class HelpManagerTest : public HelpTestBase
{
    public:
        HelpManagerTest();

        gmx::File               helpFile_;
};

HelpManagerTest::HelpManagerTest()
    : helpFile_(tempFiles_.getTemporaryFilePath("helptext.txt"), "w")
{
}

TEST_F(HelpManagerTest, HandlesRootTopic)
{
    EXPECT_CALL(rootTopic_, writeHelp(&helpFile_));
    manager_.writeCurrentTopic(&helpFile_);
}

TEST_F(HelpManagerTest, HandlesSubTopics)
{
    MockHelpTopic &first =
        rootTopic_.addSubTopic("first", "First topic", "First topic text");
    MockHelpTopic &firstSub =
        first.addSubTopic("firstsub", "First subtopic", "First subtopic text");
    rootTopic_.addSubTopic("second", "Second topic", "Second topic text");

    EXPECT_CALL(firstSub, writeHelp(&helpFile_));
    ASSERT_NO_THROW(manager_.enterTopic("first"));
    ASSERT_NO_THROW(manager_.enterTopic("firstsub"));
    manager_.writeCurrentTopic(&helpFile_);
}

TEST_F(HelpManagerTest, HandlesInvalidTopics)
{
    MockHelpTopic &first =
        rootTopic_.addSubTopic("first", "First topic", "First topic text");
    first.addSubTopic("firstsub", "First subtopic", "First subtopic text");
    rootTopic_.addSubTopic("second", "Second topic", "Second topic text");

    ASSERT_THROW(manager_.enterTopic("unknown"), gmx::InvalidInputError);
    ASSERT_NO_THROW(manager_.enterTopic("first"));
    ASSERT_THROW(manager_.enterTopic("unknown"), gmx::InvalidInputError);
    ASSERT_THROW(manager_.enterTopic("second"), gmx::InvalidInputError);
    ASSERT_NO_THROW(manager_.enterTopic("firstsub"));
}

/********************************************************************
 * Tests for help topic formatting
 */

struct TestHelpText
{
    static const char name[];
    static const char title[];
    static const char *const text[];
};

const char TestHelpText::name[] = "testtopic";
const char TestHelpText::title[] = "Topic title";
const char *const TestHelpText::text[] = {
    "Test topic text.[PAR]",
    "Another paragraph of text."
};

class HelpTopicFormattingTest : public HelpTestBase
{
    public:
        void checkHelpFormatting();
};

void HelpTopicFormattingTest::checkHelpFormatting()
{
    std::string filename = tempFiles_.getTemporaryFilePath("helptext.txt");
    ASSERT_NO_THROW(manager_.enterTopic("testtopic"));
    gmx::File file(filename, "w");
    ASSERT_NO_THROW(manager_.writeCurrentTopic(&file));
    file.close();

    checkFileContents(filename, "HelpText");
}

TEST_F(HelpTopicFormattingTest, FormatsSimpleTopic)
{
    rootTopic_.addSubTopic(gmx::HelpTopicPointer(
                new gmx::SimpleHelpTopic<TestHelpText>));
    checkHelpFormatting();
}

TEST_F(HelpTopicFormattingTest, FormatsCompositeTopicWithSubTopics)
{
    gmx::CompositeHelpTopicPointer topic(new gmx::CompositeHelpTopic<TestHelpText>);
    MockHelpTopic::addSubTopic(topic.get(), "subtopic", "First subtopic", "Text");
    MockHelpTopic::addSubTopic(topic.get(), "other", "Second subtopic", "Text");
    rootTopic_.addSubTopic(move(topic));
    checkHelpFormatting();
}

} // namespace
