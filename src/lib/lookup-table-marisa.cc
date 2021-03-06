#include <travatar/lookup-table-marisa.h>
#include <travatar/dict.h>
#include <travatar/hyper-graph.h>
#include <travatar/input-file-stream.h>
#include <travatar/global-debug.h>
#include <travatar/string-util.h>
#include <marisa/marisa.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>

using namespace travatar;
using namespace std;
using namespace boost;

// Match the start of an edge
LookupState * LookupTableMarisa::MatchStart(const HyperNode & node, const LookupState & state) const {
    const std::string & p = state.GetString();
    std::string next = p + (p.size()?" ":"") + Dict::WSym(node.GetSym()) + " (";
    return MatchState(next, state);
}

// Match the end of an edge
LookupState * LookupTableMarisa::MatchEnd(const HyperNode & node, const LookupState & state) const {
    std::string next = state.GetString() + " )";
    return MatchState(next, state);
}

LookupTableMarisa * LookupTableMarisa::ReadFromFile(std::string & filename) {
    InputFileStream tm_in(filename.c_str());
    cerr << "Reading TM file from "<<filename<<"..." << endl;
    if(!tm_in)
        THROW_ERROR("Could not find TM: " << filename);
    return ReadFromRuleTable(tm_in);
}

LookupTableMarisa * LookupTableMarisa::ReadFromRuleTable(std::istream & in) {
    // First read in the rule table
    string line, last_src;
    LookupTableMarisa * ret = new LookupTableMarisa;
    // Rule table
    typedef vector<TranslationRule*> RuleVec;
    vector<RuleVec> rules;
    marisa::Keyset keyset;
    while(getline(in, line)) {
        vector<string> columns = Tokenize(line, " ||| ");
        if(columns.size() < 3) { delete ret; THROW_ERROR("Bad line in rule table: " << line); }
        vector<WordId> trg_words, trg_syms;
        CfgDataVector trg_data = Dict::ParseAnnotatedVector(columns[1]);
        SparseVector features = Dict::ParseSparseVector(columns[2]);
        TranslationRule* rule = new TranslationRule(trg_data, features);
        if(rules.size() == 0 || columns[0] != last_src) {
            keyset.push_back(columns[0].c_str());
            rules.push_back(vector<TranslationRule*>());
            last_src = columns[0];
        }
        rules.rbegin()->push_back(rule);
    }
    // Build the trie
    ret->GetTrie().build(keyset);
    // Insert the rule arrays into the appropriate position based on the tree ID
    vector<RuleVec> & main_rules = ret->GetRules();
    main_rules.resize(keyset.size());
    for(size_t i = 0; i < rules.size(); i++) {
        const RuleVec & my_rules = rules[i];
        marisa::Agent agent;
        string str(keyset[i].ptr(), keyset[i].length());
        agent.set_query(str.c_str());
        if(!ret->GetTrie().lookup(agent))
            THROW_ERROR("Internal error when building rule table @ " << str);
        main_rules[agent.key().id()] = my_rules;
    }
    return ret;
}

// Match a single node
LookupState * LookupTableMarisa::MatchNode(const HyperNode & node, const LookupState & state) const {
    LookupState * ret = NULL;
    if(node.IsTerminal()) {
        string next = state.GetString() + " \"" + Dict::WSym(node.GetSym()) + "\""; 
        ret = MatchState(next, state);
    } else {
        ostringstream next;
        next << state.GetString() << " x" << state.GetNonterms().size() << ":" << Dict::WSym(node.GetSym());
        ret = MatchState(next.str(), state);
        if(ret != NULL)
            ret->GetNonterms().push_back(&node);
    }
    return ret;
}

LookupState * LookupTableMarisa::MatchState(const string & next, const LookupState & state) const {
    marisa::Agent agent;
    agent.set_query(next.c_str());
    if(trie_.predictive_search(agent)) {
        // cerr << "Matching " << next << " --> success!" << endl;
        LookupState * ret = new LookupState;
        ret->SetString(next);
        ret->SetNonterms(state.GetNonterms());
        ret->SetFeatures(state.GetFeatures());
        return ret;
    } else {
        // cerr << "Matching " << next << " --> failure!" << endl;
        return NULL;
    }
}


const vector<TranslationRule*> * LookupTableMarisa::FindRules(const LookupState & state) const {
    marisa::Agent agent;
    const char* query = state.GetString().c_str();
    agent.set_query(query);
    const vector<TranslationRule*> * ret = trie_.lookup(agent) ? &rules_[agent.key().id()] : NULL;
    return ret;
}


LookupTableMarisa::~LookupTableMarisa() {
    BOOST_FOREACH(std::vector<TranslationRule*> & vec, rules_)
        BOOST_FOREACH(TranslationRule * rule, vec)
            delete rule;
};
