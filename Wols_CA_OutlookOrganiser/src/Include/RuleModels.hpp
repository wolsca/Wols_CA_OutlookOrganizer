#pragma once

#include <QList>
#include <QString>
#include <cstdint>

// ==============================================================================
// RULE DEFINITION STRUCTS (Qt V3 Schema)
// ==============================================================================

struct TopicRule
{
    uint64_t       id = 0;
    QString        name;
    QString        longName;
    QString        description;
    QString        folderId;
    QList<QString> keywords;
    int            minMatchCount = 1;
};

struct OrgContactRule
{
    uint64_t id = 0;
    QString  name;
    QString  longName;
    QString  description;
    QString  email;
    QString  folderId;
};

struct OrganizationRule
{
    uint64_t              id = 0;
    QString               name;
    QString               longName;
    QString               description;
    QString               folderId;
    QList<QString>        domainCatchall;
    QList<TopicRule>      topics;
    QList<OrgContactRule> contacts;
};

struct ContactRule
{
    uint64_t       id = 0;
    QString        name;
    QString        longName;
    QString        description;
    QString        folderId;
    QList<QString> emails;
};

struct TenderPhase
{
    uint64_t id = 0;
    QString  name;
    QString  longName;
    QString  description;
    QString  folderId;
};

struct TenderRule
{
    uint64_t           id = 0;
    QString            name;
    QString            longName;
    QString            description;
    QString            folderId;
    QList<QString>     keywords;
    QList<TenderPhase> phases;
};

// ==============================================================================
// ENGINE DECISION OUTPUT
// ==============================================================================

struct RoutingDecision
{
    bool    ruleFound = false;
    QString targetFolderId;
    QString fallbackPath;
    int     priority = 4; // 1: LinkedIn/Tender, 2: Contact, 3: Org, 4: Audit
};
