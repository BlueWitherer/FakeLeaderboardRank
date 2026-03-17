#include <ranges>

#include <Geode/Geode.hpp>

#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/Button.hpp>

#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/GJAccountSettingsLayer.hpp>

using namespace geode::prelude;

static constexpr int s_zOrder = 10;

static constexpr CCPoint s_rankHintPos = {160.5f, 289.f};
static constexpr CCPoint s_rankPos = {s_rankHintPos.x, 277.f};

static constexpr float s_rankScale = 0.6f;

namespace settings {
    constexpr auto top1 = "Top 1";
    constexpr auto top10 = "Top 10";
    constexpr auto top50 = "Top 50";
    constexpr auto top100 = "Top 100";
    constexpr auto top200 = "Top 200";
    constexpr auto top500 = "Top 500";
    constexpr auto top1000 = "Top 1,000";
    constexpr auto top2500 = "Top 2,500";
    constexpr auto top5000 = "Top 5,000";
    constexpr auto top10000 = "Top 10,000";

    constexpr auto all = "All";
};

namespace nodes {
    constexpr auto label = "global-rank-label";
    constexpr auto hint = "global-rank-hint";
    constexpr auto icon = "global-rank-icon";
};

class $modify(FLRProfilePage, ProfilePage) {
    static void onModify(auto& self) {
        utils::StringMap<std::shared_ptr<Hook>>& hooks = self.m_hooks;
        for (auto& hook : hooks | std::views::values) (void)self.setHookPriorityPre(hook->getDisplayName(), Priority::FirstPre);
    };

    struct Fields {
        WeakRef<CCLabelBMFont> globalRankLabel = nullptr;

        int64_t fakeRank = Mod::get()->getSettingValue<int64_t>("rank");

        bool autoIcon = Mod::get()->getSettingValue<bool>("icon-auto");
        std::string const icon = Mod::get()->getSettingValue<std::string>("icon");

        bool forceBan = Mod::get()->getSettingValue<bool>("simulate-ban");
    };

    void loadPageFromUserInfo(GJUserScore* score) {
        auto f = m_fields.self();

        // dev stuff here :D
        score->m_globalRank = f->forceBan ? 0 : score->m_globalRank;
        ProfilePage::loadPageFromUserInfo(score);

        log::trace("set as own profile [{}] vs is current user [{}]", m_ownProfile ? "YES" : "NO", score->isCurrentUser() ? "YES" : "NO");

        if (m_ownProfile || score->isCurrentUser()) {
            WeakRef<CCSprite> globalRankIcon = nullptr;
            WeakRef<CCSprite> globalRankHint = nullptr;

            auto const rankStr = utils::numToString(f->fakeRank);

            if (score->m_globalRank <= 0) {  // check for leaderboard ban, nodes arent created if so
                log::warn("player {} is leaderboard banned!", score->m_userName);

                auto newLabel = CCLabelBMFont::create(rankStr.c_str(), "chatFont.fnt");
                newLabel->setID(nodes::label);
                newLabel->setPosition(s_rankPos);
                newLabel->setScale(s_rankScale);

                m_mainLayer->addChild(newLabel, s_zOrder);
                f->globalRankLabel = newLabel;

                auto newHint = CCSprite::createWithSpriteFrameName("gj_globalRankTxt_001.png");
                newHint->setID(nodes::hint);
                newHint->setPosition(s_rankHintPos);

                m_mainLayer->addChild(newHint, s_zOrder);
                globalRankHint = newHint;
            } else {
                log::info("player {} is on the leaderboards!", score->m_userName);

                globalRankIcon = typeinfo_cast<CCSprite*>(m_mainLayer->getChildByID(nodes::icon));
                globalRankHint = typeinfo_cast<CCSprite*>(m_mainLayer->getChildByID(nodes::hint));

                if (auto label = typeinfo_cast<CCLabelBMFont*>(m_mainLayer->getChildByID(nodes::label))) f->globalRankLabel = label;
            };

            auto newIcon = CCSprite::createWithSpriteFrameName(f->autoIcon ? getTrophySpriteByRank(f->fakeRank) : getTrophySprite(f->icon));
            newIcon->setID(nodes::icon);

            if (auto label = f->globalRankLabel.lock()) label->setString(rankStr.c_str());
            if (auto hint = globalRankHint.lock()) newIcon->setPosition({(hint->getPositionX() - hint->getScaledContentWidth()) + 3.f, 283.f});
            if (auto icon = globalRankIcon.lock()) icon->removeMeAndCleanup();

            m_mainLayer->addChild(newIcon, s_zOrder);
        } else {
            ProfilePage::loadPageFromUserInfo(score);
        };
    };

    constexpr const char* getTrophySprite(std::string_view setting) noexcept {
        if (setting == settings::top1) return "rankIcon_1_001.png";
        if (setting == settings::top10) return "rankIcon_top10_001.png";
        if (setting == settings::top50) return "rankIcon_top50_001.png";
        if (setting == settings::top100) return "rankIcon_top100_001.png";
        if (setting == settings::top200) return "rankIcon_top200_001.png";
        if (setting == settings::top500) return "rankIcon_top500_001.png";
        if (setting == settings::top1000) return "rankIcon_top1000_001.png";
        if (setting == settings::top2500) return "rankIcon_top2500_001.png";
        if (setting == settings::top5000) return "rankIcon_top5000_001.png";
        if (setting == settings::top10000) return "rankIcon_top10000_001.png";

        return "rankIcon_all_001.png";
    };

    constexpr const char* getTrophySpriteByRank(int rank) noexcept {
        if (rank <= 1) return getTrophySprite(settings::top1);
        if (rank <= 10) return getTrophySprite(settings::top10);
        if (rank <= 50) return getTrophySprite(settings::top50);
        if (rank <= 100) return getTrophySprite(settings::top100);
        if (rank <= 200) return getTrophySprite(settings::top200);
        if (rank <= 500) return getTrophySprite(settings::top500);
        if (rank <= 1000) return getTrophySprite(settings::top1000);
        if (rank <= 2500) return getTrophySprite(settings::top2500);
        if (rank <= 5000) return getTrophySprite(settings::top5000);
        if (rank <= 10000) return getTrophySprite(settings::top10000);

        return getTrophySprite(settings::all);
    };
};

class $modify(FLRGJAccountSettingsLayer, GJAccountSettingsLayer) {
    bool init(int accountID) {
        if (!GJAccountSettingsLayer::init(accountID)) return false;

        auto settingsBtn = Button::createWithNode(
            CircleButtonSprite::createWithSpriteFrameName(
                "rankIcon_1_001.png",
                1.125f,
                CircleBaseColor::Green,
                CircleBaseSize::Small),
            [](auto) {
                openSettingsPopup(Mod::get());
            });
        settingsBtn->setID("rank-settings-btn"_spr);
        settingsBtn->setScale(0.875f);
        settingsBtn->setPosition({(m_mainLayer->getScaledContentWidth() / 2.f) + 190.f, 20.f});
        settingsBtn->setTouchPriority(-999);  // fuck this layer

        m_mainLayer->addChild(settingsBtn, 9);

        return true;
    };
};