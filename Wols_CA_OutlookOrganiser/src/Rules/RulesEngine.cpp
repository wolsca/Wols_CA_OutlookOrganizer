#include "RulesEngine.hpp"

RoutingDecision RulesEngine::DetermineRoute(
    const QList<OrganizationRule> &organizations,
    const QList<ContactRule>      &contacts,
    const QList<TenderRule>       &tenders,
    const QString                 &senderEmail,
    const QString                 &subject,
    const QString                 &body )
{
    QString lowerEmail       = senderEmail.toLower();
    QString lowerSubjectBody = ( subject + " " + body ).toLower();

    if ( auto tenderDecision = EvaluateTenders( tenders, lowerSubjectBody ) )
    {
        return tenderDecision.value();
    }

    if ( auto contactDecision = EvaluatePersonalContacts( contacts, lowerEmail ) )
    {
        return contactDecision.value();
    }

    if ( auto orgDecision = EvaluateOrganizations( organizations, lowerEmail, lowerSubjectBody ) )
    {
        return orgDecision.value();
    }

    RoutingDecision decision;
    decision.ruleFound = false;
    decision.priority  = 4;
    return decision;
}

std::optional<RoutingDecision> RulesEngine::EvaluateTenders( const QList<TenderRule> &tenders, const QString &lowerSubjectBody )
{
    for ( const auto &tender : tenders )
    {
        if ( ContainsKeywords( lowerSubjectBody, tender.keywords, 1 ) )
        {
            RoutingDecision decision;
            decision.ruleFound      = true;
            decision.priority       = 1;
            decision.targetFolderId = tender.folderId;
            decision.fallbackPath   = "00_Organizations/" + tender.name;
            return decision;
        }
    }
    return std::nullopt;
}

std::optional<RoutingDecision> RulesEngine::EvaluatePersonalContacts( const QList<ContactRule> &contacts, const QString &lowerEmail )
{
    for ( const auto &contact : contacts )
    {
        for ( const auto &email : contact.emails )
        {
            if ( lowerEmail == email.toLower() )
            {
                RoutingDecision decision;
                decision.ruleFound      = true;
                decision.priority       = 2;
                decision.targetFolderId = contact.folderId;
                decision.fallbackPath   = "00_Contacts/" + contact.name;
                return decision;
            }
        }
    }
    return std::nullopt;
}

std::optional<RoutingDecision> RulesEngine::EvaluateOrganizations( const QList<OrganizationRule> &organizations, const QString &lowerEmail, const QString &lowerSubjectBody )
{
    for ( const auto &org : organizations )
    {
        bool isOrgMatch = false;

        for ( const auto &orgContact : org.contacts )
        {
            if ( lowerEmail == orgContact.email.toLower() )
            {
                RoutingDecision decision;
                decision.ruleFound      = true;
                decision.priority       = 2;
                decision.targetFolderId = orgContact.folderId;
                decision.fallbackPath   = "00_Organizations/" + org.name + "/" + orgContact.name;
                return decision;
            }
        }

        for ( const auto &domain : org.domainCatchall )
        {
            if ( lowerEmail.contains( domain.toLower() ) )
            {
                isOrgMatch = true;
                break;
            }
        }

        if ( isOrgMatch )
        {
            if ( auto topicDecision = EvaluateTopics( org, lowerSubjectBody ) )
            {
                return topicDecision.value();
            }

            RoutingDecision decision;
            decision.ruleFound      = true;
            decision.priority       = 3;
            decision.targetFolderId = org.folderId;
            decision.fallbackPath   = "00_Organizations/" + org.name;
            return decision;
        }
    }
    return std::nullopt;
}

std::optional<RoutingDecision> RulesEngine::EvaluateTopics( const OrganizationRule &org, const QString &lowerSubjectBody )
{
    for ( const auto &topic : org.topics )
    {
        if ( ContainsKeywords( lowerSubjectBody, topic.keywords, topic.minMatchCount ) )
        {
            RoutingDecision decision;
            decision.ruleFound      = true;
            decision.priority       = 3;
            decision.targetFolderId = topic.folderId;
            decision.fallbackPath   = "00_Organizations/" + org.name + "/" + topic.name;
            return decision;
        }
    }
    return std::nullopt;
}

bool RulesEngine::ContainsKeywords( const QString &text, const QList<QString> &keywords, int minMatch )
{
    int matchCount = 0;
    for ( const auto &kw : keywords )
    {
        // Qt regelt de case-insensitive check direct intern, extreem efficiënt
        if ( text.contains( kw, Qt::CaseInsensitive ) )
        {
            matchCount++;
            if ( matchCount >= minMatch )
            {
                return true;
            }
        }
    }
    return false;
}
