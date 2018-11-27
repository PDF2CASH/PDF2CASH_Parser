#pragma once

#ifndef SEARCH_H
#define SEARCH_H

#include <QString>
#include <QStringList>

#include <QMap>
#include <QList>
#include <QVector>
#include <QRect>

#include "Parser.h"

//struct pdf2cash::sTEXTDATA;

struct TrieNode
{
    // The set with all the letters which this node is prefix
    QMap<QChar, TrieNode*> next;

    // If word is equal to "" is because there is no word in the
    //  dictionary which ends here.
    QString word;

    TrieNode() : next(QMap<QChar, TrieNode*>()) {}

    void insert(QString w)
    {
        w = QString("$") + w;

        int sz = w.size();

        TrieNode* n = this;
        for (int i = 0; i < sz; ++i)
        {
            if (n->next.find(w[i]) == n->next.end())
            {
                n->next[w[i]] = new TrieNode();
            }

            n = n->next[w[i]];
        }

        n->word = w;
    }
};

struct SearchDataDistance
{
    int rate;
    pdf2cash::sTEXTDATA* data;

    SearchDataDistance(int r, pdf2cash::sTEXTDATA* d)
    {
        rate = r;
        data = d;
    }
};

class Search
{
public:
    Search();

    void Initialization();

    QString Convert(QString str, bool useAbbreviation = true);

    // -------------------------------------------
    // Function's related to search string's.
    // -------------------------------------------

    // Main search.
    pdf2cash::sTEXTDATA* SearchText(const QString pattern, const QList<pdf2cash::sTEXTDATA*> strList, const int averageLevenstein, const QString mainPattern = "", const bool checkDistance = false, const QRect* rect = nullptr);
    pdf2cash::sTEXTDATA* CalculeDistance(const QRect rect, const QList<pdf2cash::sTEXTDATA*> dataList);

    // Exact search.
    bool SearchByExact(const QString pattern, QString str);

    // Levenstein algorithm.
    int SearchByLevenstein(TrieNode* node, QString word, int minCost = 0x3f3f3f3f);

    // KMP algorithm.
    bool SearchByKMP(QString pattern, QString str);

    int SearchByTags(QStringList pattern, QString str, QString mainPattern = "");

    // -------------------------------------------
    // Function's related to test.
    // -------------------------------------------
    void TestByLevenstein();
    void TestByKMP();

private:
    // -------------------------------------------
    // Function's related to edit string's.
    // -------------------------------------------
    bool ToLowerCase(QString* str);
    bool RemoveAccents(QString* str);
    bool RemoveSpecialCharacter(QString* str);
    bool RemoveExtraCharacter(QString* str, const QChar c);

    bool RemoveAbbreviation(QString* str);

    bool RemoveAbnormal(QString* str);

    // -------------------------------------------
    // Function's related to search string's.
    // -------------------------------------------

    // Levenstein algorithm.
    void SearchImpl(TrieNode* tree, QChar ch, QVector<int> last_row, const QString& word, int* minCost);

    // KMP algorithm.
    void ComputeLPSArray(QString pat, int M, int* lps);

    // -------------------------------------------
    // Function's related to test.
    // -------------------------------------------
    QList<QString>* CreateListTest();

private:
    QMap<QString, QList<QString>> _abbreviationsMap;
};

#endif // SEARCH_H
