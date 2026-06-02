#pragma once

#include "RuleModels.hpp"
#include <QList>
#include <QString>
#include <optional>

// ==============================================================================
// RULES ENGINE (Stateless Qt Routing Logic)
// ==============================================================================

class RulesEngine
{
  public:
    static RoutingDecision DetermineRoute(
        const QList<OrganizationRule> &organizations,
        const QList<ContactRule>      &contacts,
        const QList<TenderRule>       &tenders,
        const QString                 &senderEmail,
        const QString                 &subject,
        const QString                 &body );

  private:
    static std::optional<RoutingDecision> EvaluateTenders(
        const QList<TenderRule> &tenders,
        const QString           &lowerSubjectBody );

    static std::optional<RoutingDecision> EvaluatePersonalContacts(
        const QList<ContactRule> &contacts,
        const QString            &lowerEmail );

    static std::optional<RoutingDecision> EvaluateOrganizations(
        const QList<OrganizationRule> &organizations,
        const QString                 &lowerEmail,
        const QString                 &lowerSubjectBody );

    static std::optional<RoutingDecision> EvaluateTopics(
        const OrganizationRule &org,
        const QString          &lowerSubjectBody );

    static bool ContainsKeywords(
        const QString        &text,
        const QList<QString> &keywords,
        int                   minMatch );
};
