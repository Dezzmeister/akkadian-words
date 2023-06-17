#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <Windows.h>

#include "common.h"
#include "errors.h"
#include "practice.h"

void WordPracticeState::reset() {
    correct = 0;
    total = 0;
    word = L"";
}

void WordPracticeState::new_word(Dictionary& dict, bool engl) {
    std::pair<std::wstring, DictEntry> item = engl ? dict.random_engl() : dict.random_akk();
    word = item.first;
    entry = item.second;
}

bool WordPracticeState::accept_answer(std::wstring& answer) {
    total++;

    if (entry.has_defn(answer)) {
        correct++;
        return true;
    }

    return false;
}

std::wstring WordPracticeState::get_summary(Dictionary& dict, bool engl, bool was_correct, std::wstring answer) {
    if (total == 0) {
        return L"0/0";
    }

    DictEntry entry = this->entry;

    // Find the Akkadian word's definition
    if (engl && was_correct) {
        const std::optional<const std::vector<DictEntry>*> entries_opt = dict.get_akk(answer);
        const std::vector<DictEntry>* entries = *entries_opt;

        for (const DictEntry& e : *entries) {
            if (e.grammar_kind == entry.grammar_kind && e.word_types == entry.word_types) {
                entry = e;
                this->word = answer;
                break;
            }
        }

        // This is unreachable because at this point we know that the answer exists in the dictionary
        __assume(false);
    }

    std::wostringstream score;
    score << std::fixed << std::setprecision(2) << ((correct / (double)total) * 100);

    std::wstring out = std::to_wstring(correct) + L"/" + std::to_wstring(total)
        + L" (" + score.str() + L"%) ";

    out += get_question() += L":\r\n";

    for (size_t i = 0; i < entry.defns.size() - 1; i++) {
        std::wstring defn = entry.defns[i];
        out += defn + L", ";
    }

    out += entry.defns[entry.defns.size() - 1];

    return out;
}

std::wstring WordPracticeState::get_question() {
    return word + L" " + entry.get_attrs();
}

PhraseAnswer::PhraseAnswer(bool correct, std::wstring noun, std::wstring adj) : correct(correct), noun(noun), adj(adj) {}

void PhrasePracticeState::reset() {
    correct = 0;
    total = 0;
    noun_word = L"";
    adj_word = L"";
}

void PhrasePracticeState::new_phrase(Dictionary& dict, bool engl) {
    word_case = (WordCase)(rand() % NUM_WORD_CASES);
    WordClass gender = (rand() % 2 == 0) ? WordClass::Masculine : WordClass::Feminine;
    WordClass noun_num = (rand() % 2 == 0) ? WordClass::Singular : WordClass::Plural;
    WordClass adj_num = noun_num;
    GrammarKind adj_grammar_kind = GrammarKind::Adjective;

    // Filter out any words that have more than one word or some kind of qualifier to prevent phrases
    // like "having arrived lords"
    static FilterFunc phrase_filter = [](const std::wstring& word, const DictEntry& entry) {
        // Search only for left paren: assume dictionary is well formed. The worst that can happen
        // is the user sees a weird prompt
        if (word.find(' ') != std::wstring::npos || word.find('(') != std::wstring::npos) {
            return false;
        }

        // Find one definition that has no spaces or parentheses, so that the correct answer won't be
        // "having arrived lords"
        for (const std::wstring& defn : entry.defns) {
            if (defn.find(' ') == std::wstring::npos && defn.find('(') == std::wstring::npos) {
                return true;
            }
        }

        return false;
    };

    // Make noun dual case, adjective plural feminine
    if ((rand() % 3) == 0) {
        gender = WordClass::Feminine;
        noun_num = WordClass::Dual;
        adj_num = WordClass::Plural;
    }

    // Use the anaphoric pronoun instead of an adjective
    if ((rand() % 50) == 0) {
        adj_grammar_kind = GrammarKind::AnaphoricPronoun;
    }

    std::optional<std::pair<std::wstring, DictEntry>> noun_opt;
    std::optional<std::pair<std::wstring, DictEntry>> adj_opt;

    switch (word_case) {
    case WordCase::Nom: {
        noun_opt = dict.rand_filters(GrammarKind::Noun, { WordClass::Nominative, gender, noun_num }, {}, engl, phrase_filter);
        adj_opt = dict.rand_filters(adj_grammar_kind, { WordClass::Nominative, gender, adj_num }, {}, engl, phrase_filter);
        break;
    }
    case WordCase::Gen: {
        noun_opt = dict.rand_filters(GrammarKind::Noun, { gender, noun_num }, { WordRelationKind::GenitiveOf }, engl, phrase_filter);
        adj_opt = dict.rand_filters(adj_grammar_kind, { gender, adj_num }, { WordRelationKind::GenitiveOf }, engl, phrase_filter);
        break;
    }
    case WordCase::Acc: {
        noun_opt = dict.rand_filters(GrammarKind::Noun, { gender, noun_num }, { WordRelationKind::AccusativeOf }, engl, phrase_filter);
        adj_opt = dict.rand_filters(adj_grammar_kind, { gender, adj_num }, { WordRelationKind::AccusativeOf }, engl, phrase_filter);
        break;
    }
    }

    if (!noun_opt.has_value() || !adj_opt.has_value()) {
        throw PracticeError(PracticeErrorType::NotEnoughCases);
    }

    auto& [noun_word, noun_entry] = *noun_opt;
    auto& [adj_word, adj_entry] = *adj_opt;

    this->noun_word = noun_word;
    this->noun_entry = noun_entry;
    this->adj_word = adj_word;
    this->adj_entry = adj_entry;
}

PhraseAnswer PhrasePracticeState::accept_answer(std::wstring& answer, bool engl) {
    size_t space_pos = answer.find(' ');

    total++;

    if (space_pos == std::wstring::npos) {
        return PhraseAnswer(false, L"", L"");
    }

    std::wstring second_ans = answer.substr(0, space_pos);
    std::wstring first_ans = answer.substr(space_pos + 1);

    std::wstring& noun_ans = first_ans;
    std::wstring& adj_ans = second_ans;

    if (engl) {
        noun_ans = second_ans;
        adj_ans = first_ans;
    }

    OutputDebugStringW((L"noun ans: " + noun_ans + L"\n").c_str());
    OutputDebugStringW((L"adj ans: " + adj_ans + L"\n").c_str());

    if (noun_entry.has_defn(noun_ans) && adj_entry.has_defn(adj_ans)) {
        correct++;
        return PhraseAnswer(true, first_ans, second_ans);
    }

    return PhraseAnswer(false, L"", L"");
}

std::wstring PhrasePracticeState::get_summary(Dictionary& dict, bool engl, bool was_correct, std::wstring noun_answer, std::wstring adj_answer) {
    if (total == 0) {
        return L"0/0";
    }

    DictEntry& noun = noun_entry;
    DictEntry& adj = adj_entry;

    if (engl && was_correct) {
        std::optional<DictEntry*> noun_opt = dict.get_akk_filters(noun_answer, { noun_entry.grammar_kind }, noun_entry.word_types);
        std::optional<DictEntry*> adj_opt = dict.get_akk_filters(adj_answer, { adj_entry.grammar_kind }, adj_entry.word_types);

        noun = **noun_opt;
        adj = **adj_opt;
    }

    std::wostringstream score;
    score << std::fixed << std::setprecision(2) << ((correct / (double)total) * 100);

    std::wstring out = std::to_wstring(correct) + L"/" + std::to_wstring(total)
        + L" (" + score.str() + L"%) ";

    out += get_question(engl) += L":\r\n";

    out += noun.akk_summary(noun_word) + L"\r\n";
    out += adj.akk_summary(adj_word);

    return out;
}

std::wstring PhrasePracticeState::get_question(bool engl) {
    bool masc = noun_entry.has_word_classes({ WordClass::Masculine });
    bool sing = noun_entry.has_word_classes({ WordClass::Singular });
    bool dual = noun_entry.has_word_classes({ WordClass::Dual });

    std::wstring out = L"";

    if (engl) {
        out += adj_word + L" " + noun_word;
    }
    else {
        out += noun_word + L" " + adj_word;
    }

    out += L" (";

    if (masc) {
        out += L"m, ";
    }
    else {
        out += L"f, ";
    }

    if (sing) {
        out += L"s, ";
    }
    else if (dual) {
        out += L"dual, ";
    }
    else {
        out += L"pl, ";
    }

    switch (word_case) {
    case WordCase::Nom: {
        out += L"nom)";
        break;
    }
    case WordCase::Gen: {
        out += L"gen)";
        break;
    }
    case WordCase::Acc: {
        out += L"acc)";
        break;
    }
    }

    return out;
}
