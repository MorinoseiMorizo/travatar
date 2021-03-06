#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <travatar/binarizer-cky.h>
#include <travatar/hyper-graph.h>
#include <travatar/dict.h>

using namespace travatar;
using namespace std;
using namespace boost;

typedef BinarizerCKY::SNMap SNMap;
HyperNode * BinarizerCKY::FindIndexedNode(const HyperGraph & hg, HyperGraph & ret, SNMap & snmap, const vector<int> & str, WordId xbar) const {
    SNMap::const_iterator it = snmap.find(str);
    if(it != snmap.end()) return it->second;
    HyperNode * new_node = new HyperNode;
    for(int i = 0; i < (int)str.size(); i++) {
        const HyperNode* old_node = hg.GetNode(str[i]);
        new_node->AddSpan(old_node->GetSpan());
        new_node->SetSym(old_node->GetSym());
    }
    if(str.size() > 1) new_node->SetSym(xbar);
    ret.AddNode(new_node);
    snmap.insert(make_pair(str, new_node));
    return new_node;
    
}

// Binarize the graph to the right
HyperGraph * BinarizerCKY::TransformGraph(const HyperGraph & hg) const {
    // First copy the graph
    HyperGraph * ret = new HyperGraph;
    ret->SetWords(hg.GetWords());
    // A map to keep track of nodes that have already been built for this graph
    SNMap built_nodes;
    // Process each edge in the original graph
    BOOST_FOREACH(const HyperEdge * edge, hg.GetEdges()) {
        // Get the symbol for x-bar
        WordId xbar = Dict::WID(Dict::WSym(edge->GetHead()->GetSym())+"'");
        vector<int> head_str(1);
        head_str[0] = edge->GetHead()->GetId();
        const vector<HyperNode*> & tails = edge->GetTails();
        vector<int> tail_str(tails.size());
        for(int i = 0; i < (int)edge->GetTails().size(); i++)
            tail_str[i] = edge->GetTail(i)->GetId();
        // We need to deal with two cases: binarization is necessary or not
        // First, we handle the "not" case, as it is simpler
        if(edge->NumTails() <= 2 || edge->NumTails() > max_tails_) {
            HyperEdge * new_edge = new HyperEdge(*edge);
            new_edge->SetHead(FindIndexedNode(hg, *ret, built_nodes, head_str, xbar));
            for(int i = 0; i < (int)edge->GetTails().size(); i++) {
                tail_str[i] = edge->GetTail(i)->GetId();
                new_edge->GetTails()[i] = FindIndexedNode(hg, *ret, built_nodes, VectorSubstr(tail_str, i, 1), xbar);
            }
            ret->AddEdge(new_edge); 
            new_edge->GetHead()->AddEdge(new_edge);
            continue;
        }
        // Next, we handle the case where binarization is necessary
        // Create a generic string holding the tails
        for(int i = 0; i < (int)tails.size(); i++)
            tail_str[i] = tails[i]->GetId();
        // Binarize until we have reached an intermediate node that is finished
        for(int j = 2; j <= (int)tails.size(); j++) {
            for(int i = 0; i < j-1; i++) {
                bool is_top = (j-i == (int)tails.size());
                vector<int> my_head_str = (is_top ? head_str : VectorSubstr(tail_str, i, j-i));
                HyperNode* head = FindIndexedNode(hg, *ret, built_nodes, my_head_str, xbar);
                // Skip nodes that have already been made
                if(!is_top && head->GetEdges().size() > 0)
                    continue;
                // Binarize
                for(int k = i+1; k < j; k++) {
                    // Skip the middle nodes
                    if(k != i+1) k = j-1;
                    // Find the left and right nodes
                    HyperNode* left = FindIndexedNode(hg, *ret, built_nodes, VectorSubstr(tail_str, i, k-i), xbar);
                    HyperNode* right = FindIndexedNode(hg, *ret, built_nodes, VectorSubstr(tail_str, k, j-k), xbar);
                    // Create the left and right edges
                    HyperEdge * next_edge = new HyperEdge(head);
                    next_edge->AddTail(left);
                    next_edge->AddTail(right);
                    // Score only the top edge
                    if(is_top) {
                        next_edge->SetFeatures(edge->GetFeatures());
                        next_edge->SetScore(edge->GetScore());
                    }
                    ret->AddEdge(next_edge); head->AddEdge(next_edge);
                }
            }
        }
    }
    // cerr << "BinarizerCKY returns nodes: " << ret->NumNodes() << ", edges: " << ret->NumEdges() << endl;
    return ret;
}
