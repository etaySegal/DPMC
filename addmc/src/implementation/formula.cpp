/* inclusions *****************************************************************/

#include <set>

#include "../interface/formula.hpp"

/* constants ******************************************************************/

const string &CNF_WORD = "cnf";
const string &WCNF_WORD = "wcnf";
const string &WPCNF_WORD = "wpcnf";
const string &WEIGHTS_WORD = "weights";
const string &WEIGHT_WORD = "w";
const string &LINE_END_WORD = "0";

/* classes ********************************************************************/

/* class Label ****************************************************************/

void Label::addNumber(Int i) {
  push_back(i);
  std::sort(begin(), end(), std::greater<Int>());
}

/* class Cnf ******************************************************************/

string Cnf::getWeightFormatWord(WeightFormat weightFormat) {
  switch (weightFormat) {
    case WeightFormat::WCNF: {
      return WCNF_WORD;
    }
    case WeightFormat::WPCNF: {
      return WPCNF_WORD;
    }
    default: {
      return CNF_WORD;
    }
  }
}

void Cnf::updateApparentVars(Int literal) {
  Int var = util::getCnfVar(literal);
  if (!util::isFound(var, apparentVars)) apparentVars.push_back(var);
}

void Cnf::addClause(const vector<Int> &clause) {
  clauses.push_back(clause);
  for (Int literal : clause) updateApparentVars(literal);
}

void Cnf::checkLiteralWeights() const {
  for (Int var = 1; var <= declaredVarCount; var++) {
    for (Int lit : vector<Int>{var, -var}) {
      Float weight = literalWeights.at(lit);
      if (weight <= 0) {
        showWarning("literal " + to_string(lit) + " has weight " + to_string(weight));
      }
    }
  }
}

Graph Cnf::getGaifmanGraph() const {
  Set<Int> vars;
  for (Int var : apparentVars) vars.insert(var);
  Graph graph(vars);

  for (const vector<Int> &clause : clauses) {
    for (auto literal1 = clause.begin(); literal1 != clause.end(); literal1++) {
      for (auto literal2 = std::next(literal1); literal2 != clause.end(); literal2++) {
        Int var1 = util::getCnfVar(*literal1);
        Int var2 = util::getCnfVar(*literal2);
        graph.addEdge(var1, var2);
      }
    }
  }

  return graph;
}

vector<Int> Cnf::getAppearanceVarOrdering() const {
  return apparentVars;
}

vector<Int> Cnf::getDeclarationVarOrdering() const {
  vector<Int> varOrdering = apparentVars;
  std::sort(varOrdering.begin(), varOrdering.end());
  return varOrdering;
}

vector<Int> Cnf::getRandomVarOrdering() const {
  vector<Int> varOrdering = apparentVars;
  util::shuffleRandomly(varOrdering);
  return varOrdering;
}

vector<Int> Cnf::getMcsVarOrdering() const {
  Graph graph = getGaifmanGraph();

  auto startVertex = graph.beginVertices();
  if (startVertex == graph.endVertices()) return vector<Int>();

  Map<Int, Int> rankedNeighborCounts; // unranked vertex |-> number of ranked neighbors
  for (auto it = std::next(startVertex); it != graph.endVertices(); it++) rankedNeighborCounts[*it] = 0;

  Int bestVertex = *startVertex;
  Int bestRankedNeighborCount = DUMMY_MIN_INT;

  vector<Int> varOrdering;
  do {
    varOrdering.push_back(bestVertex);

    rankedNeighborCounts.erase(bestVertex);

    for (auto n = graph.beginNeighbors(bestVertex); n != graph.endNeighbors(bestVertex); n++) {
      auto entry = rankedNeighborCounts.find(*n);
      if (entry != rankedNeighborCounts.end()) entry->second++;
    }

    bestRankedNeighborCount = DUMMY_MIN_INT;
    for (const std::pair<Int, Int> &entry : rankedNeighborCounts) {
      if (entry.second > bestRankedNeighborCount) {
        bestRankedNeighborCount = entry.second;
        bestVertex = entry.first;
      }
    }
  }
  while (bestRankedNeighborCount != DUMMY_MIN_INT);

  return varOrdering;
}

vector<Int> Cnf::getLexpVarOrdering() const {
  Map<Int, Label> unnumberedVertices;
  for (Int vertex : apparentVars) unnumberedVertices[vertex] = Label();
  vector<Int> numberedVertices; // whose alpha numbers are decreasing
  Graph graph = getGaifmanGraph();
  for (Int number = apparentVars.size(); number > 0; number--) {
    auto vertexIt = max_element(unnumberedVertices.begin(), unnumberedVertices.end(), util::isLessValued<Int, Label>);
    Int vertex = vertexIt->first; // ignores label
    numberedVertices.push_back(vertex);
    unnumberedVertices.erase(vertex);
    for (auto neighborIt = graph.beginNeighbors(vertex); neighborIt != graph.endNeighbors(vertex); neighborIt++) {
      Int neighbor = *neighborIt;
      auto unnumberedNeighborIt = unnumberedVertices.find(neighbor);
      if (unnumberedNeighborIt != unnumberedVertices.end()) {
        Int unnumberedNeighbor = unnumberedNeighborIt->first;
        unnumberedVertices.at(unnumberedNeighbor).addNumber(number);
      }
    }
  }
  return numberedVertices;
}

vector<Int> Cnf::getLexmVarOrdering() const {
  Map<Int, Label> unnumberedVertices;
  for (Int vertex : apparentVars) unnumberedVertices[vertex] = Label();
  vector<Int> numberedVertices; // whose alpha numbers are decreasing
  Graph graph = getGaifmanGraph();
  for (Int i = apparentVars.size(); i > 0; i--) {
    auto vIt = max_element(unnumberedVertices.begin(), unnumberedVertices.end(), util::isLessValued<Int, Label>);
    Int v = vIt->first; // ignores label
    numberedVertices.push_back(v);
    unnumberedVertices.erase(v);

    /* updates numberedVertices: */
    Graph subgraph = getGaifmanGraph(); // will only contain v, w, and unnumbered vertices whose labels are less than w's
    for (auto wIt = unnumberedVertices.begin(); wIt != unnumberedVertices.end(); wIt++) {
      Int w = wIt->first;
      Label &wLabel = wIt->second;

      /* removes numbered vertices except v: */
      for (Int numberedVertex : numberedVertices) {
        if (numberedVertex != v) {
          subgraph.removeVertex(numberedVertex);
        }
      }

      /* removes each non-w unnumbered vertex whose label is not less than w's */
      for (const std::pair<Int, Label> &kv : unnumberedVertices) {
        Int unnumberedVertex = kv.first;
        const Label &label = kv.second;
        if (unnumberedVertex != w && label >= wLabel) {
          subgraph.removeVertex(unnumberedVertex);
        }
      }

      if (subgraph.hasPath(v, w)) {
        wLabel.addNumber(i);
      }
    }
  }
  return numberedVertices;
}

vector<Int> Cnf::getMinfillVarOrdering() const {
  vector<Int> varOrdering;

  Graph graph = getGaifmanGraph();
  while (graph.beginVertices() != graph.endVertices()) {
    Int vertex = graph.getMinfillVertex();
    graph.fillInEdges(vertex);
    graph.removeVertex(vertex);
    varOrdering.push_back(vertex);
  }

  return varOrdering;
}

vector<Int> Cnf::getVarOrdering(VarOrderingHeuristic varOrderingHeuristic, bool inverseVarOrdering) const {
  vector<Int> varOrdering;
  switch (varOrderingHeuristic) {
    case VarOrderingHeuristic::APPEARANCE: {
      varOrdering = getAppearanceVarOrdering();
      break;
    }
    case VarOrderingHeuristic::DECLARATION: {
      varOrdering = getDeclarationVarOrdering();
      break;
    }
    case VarOrderingHeuristic::RANDOM: {
      varOrdering = getRandomVarOrdering();
      break;
    }
    case VarOrderingHeuristic::MCS: {
      varOrdering = getMcsVarOrdering();
      break;
    }
    case VarOrderingHeuristic::LEXP: {
      varOrdering = getLexpVarOrdering();
      break;
    }
    case VarOrderingHeuristic::LEXM: {
      varOrdering = getLexmVarOrdering();
      break;
    }
    case VarOrderingHeuristic::MINFILL: {
      varOrdering = getMinfillVarOrdering();
      break;
    }
    default: {
      showError("Cnf::getVarOrdering");
    }
  }
  if (inverseVarOrdering) {
    util::invert(varOrdering);
  }
  return varOrdering;
}

vector<Int> Cnf::getRestrictedVarOrdering(VarOrderingHeuristic varOrderingHeuristic, bool inverseVarOrdering, const Set<Int> &restrictedVars) const {
  vector<Int> restrictedVarOrdering;
  for (Int var : getVarOrdering(varOrderingHeuristic, inverseVarOrdering)) {
    if (util::isFound(var, restrictedVars)) {
      restrictedVarOrdering.push_back(var);
    }
  }
  return restrictedVarOrdering;
}

Int Cnf::getDeclaredVarCount() const { return declaredVarCount; }

Map<Int, Float> Cnf::getLiteralWeights() const { return literalWeights; }

Int Cnf::getEmptyClauseIndex() const {
  for (Int clauseIndex = 0; clauseIndex < clauses.size(); clauseIndex++) {
    if (clauses.at(clauseIndex).empty()) {
      return clauseIndex;
    }
  }
  return DUMMY_MIN_INT;
}

const vector<vector<Int>> &Cnf::getClauses() const { return clauses; }

const vector<Int> &Cnf::getApparentVars() const { return apparentVars; }

const Set<Int> &Cnf::getAdditiveVars() const { return additiveVars; }

Set<Int> Cnf::getDisjunctiveVars() const {
  Set<Int> disjunctiveVars;
  for (Int var = 1; var <= declaredVarCount; var++) {
    if (!util::isFound(var, additiveVars)) {
      disjunctiveVars.insert(var);
    }
  }
  return disjunctiveVars;
}

void Cnf::printAdditiveVars() const {
  std::set<Int> vars(additiveVars.begin(), additiveVars.end());
  cout << "c additiveVars:";
  for (Int var : vars) {
    cout << " " << var;
  }
  cout << "\n";
}

void Cnf::printLiteralWeights() const {
  util::printLiteralWeights(literalWeights);
}

void Cnf::printClauses() const {
  util::printCnf(clauses);
}

Cnf::Cnf(const vector<vector<Int>> &clauses) {
  for (const vector<Int> &clause : clauses) {
    addClause(clause);
    for (Int literal : clause) {
      additiveVars.insert(util::getCnfVar(literal));
    }
  }
  declaredVarCount = apparentVars.size();
}

Cnf::Cnf(const string &filePath, WeightFormat weightFormat) {
  signal(SIGINT, util::handleSignal); // Ctrl c
  signal(SIGTERM, util::handleSignal); // timeout

  printComment("processing cnf formula...", 1);

  std::ifstream inputFileStream(filePath); // variable will be destroyed if it goes out of scope
  std::istream *inputStream;
  if (filePath == STDIN_CONVENTION) {
    inputStream = &std::cin;

    printThickLine();
    printComment("getting cnf from stdin... (end input with 'Enter' then 'Ctrl d')");
  }
  else {
    if (!inputFileStream.is_open()) {
      showError("unable to open file '" + filePath + "'");
    }
    inputStream = &inputFileStream;
  }

  this->weightFormat = weightFormat;

  Int declaredClauseCount = DUMMY_MIN_INT;
  Int processedClauseCount = 0;

  Int lineIndex = 0;
  Int problemLineIndex = DUMMY_MIN_INT;
  Int minic2dWeightsLineIndex = DUMMY_MIN_INT;

  string line;
  while (std::getline(*inputStream, line)) {
    lineIndex++;
    std::istringstream inputStringStream(line);

    if (verbosityLevel >= 3) printComment("line " + to_string(lineIndex) + "\t" + line);

    vector<string> words;
    std::copy(std::istream_iterator<string>(inputStringStream), std::istream_iterator<string>(), std::back_inserter(words));

    Int wordCount = words.size();

    if (wordCount < 1) continue;

    const string &startWord = words.at(0);
    if (startWord == PROBLEM_WORD) {
      if (problemLineIndex != DUMMY_MIN_INT) {
        showError("multiple problem lines: " + to_string(problemLineIndex) + " and " + to_string(lineIndex));
      }
      problemLineIndex = lineIndex;

      if (wordCount != 4) {
        showError("problem line " + to_string(lineIndex) + " has " + to_string(wordCount) + " words (should be 4)");
      }

      const string &cnfKey = getWeightFormatWord(weightFormat);
      const string &cnfWord = words.at(1);
      if (cnfKey != cnfWord) {
        showWarning("expected '" + cnfKey + "', found '" + cnfWord + "' | line " + to_string(lineIndex));
      }

      declaredVarCount = std::stoll(words.at(2));
      declaredClauseCount = std::stoll(words.at(3));
    }
    else if (startWord == "vp") { // additive-variable line
      if (weightFormat == WeightFormat::WPCNF) {
        for (Int i = 1; i < wordCount; i++) {
          Int num = std::stoll(words.at(i));

          if (num < 0 || num > declaredVarCount) {
            showError("var '" + to_string(num) + "' inconsistent with declared var count '" + to_string(declaredVarCount) + "' | line " + to_string(lineIndex));
          }

          if (num == 0) {
            if (i != wordCount - 1) {
              showError("additive variables terminated prematurely by '0' | line " + to_string(lineIndex));
            }
          }
          else {
            additiveVars.insert(num);
          }
        }
      }
    }
    else if (startWord == "c") { // comment
      if (weightFormat == WeightFormat::MINIC2D && wordCount > 1 && words.at(1) == WEIGHTS_WORD) { // MINIC2D weights line
        if (problemLineIndex == DUMMY_MIN_INT) {
          showError("no problem line before MINIC2D weights | line " + to_string(lineIndex));
        }
        if (minic2dWeightsLineIndex != DUMMY_MIN_INT) {
          showError("multiple MINIC2D weights lines: " + to_string(minic2dWeightsLineIndex) + " and " + to_string(lineIndex));
        }
        minic2dWeightsLineIndex = lineIndex;

        if (wordCount != 2 + declaredVarCount * 2) {
          showError("wrong number of MINIC2D literal weights | line " + to_string(lineIndex));
        }

        for (Int var = 1; var <= declaredVarCount; var++) {
          literalWeights[var] = std::stold(words.at(var * 2));
          literalWeights[-var] = std::stold(words.at(var * 2 + 1));
        }
      }
    }
    else if (startWord == WEIGHT_WORD) {
      if (problemLineIndex == DUMMY_MIN_INT) {
        showError("no problem line before weight | line " + to_string(lineIndex));
      }

      if (weightFormat == WeightFormat::CACHET && (wordCount == 3 || wordCount == 4 && words.at(3) == LINE_END_WORD)) {
        Int var = std::stoll(words.at(1));
        if (var <= 0 || var > declaredVarCount) {
          showError("var '" + to_string(var) + "' inconsistent with declared var count '" + to_string(declaredVarCount) + "' | line " + to_string(lineIndex));
        }
        Float weight = std::stold(words.at(2));
        literalWeights[var] = weight;
      }
      else if ((weightFormat == WeightFormat::WCNF || weightFormat == WeightFormat::WPCNF) && (wordCount == 3 || wordCount == 4 && words.at(3) == LINE_END_WORD)) {
        Int literal = std::stoll(words.at(1));

        Int var = util::getCnfVar(literal);
        if (var <= 0 || var > declaredVarCount) {
          showError("literal '" + to_string(literal) + "' inconsistent with declared var count '" + to_string(declaredVarCount) + "' | line " + to_string(lineIndex));
        }

        Float weight = std::stold(words.at(2));
        literalWeights[literal] = weight;
      }
      else {
        showError("weight inconsistent with weight format " + util::getWeightFormatName(weightFormat) + " | line " + to_string(lineIndex));
      }
    }
    else { // clause line
      if (problemLineIndex == DUMMY_MIN_INT) {
        showError("no problem line before clause line " + to_string(lineIndex));
      }

      vector<Int> clause;
      for (Int i = 0; i < wordCount; i++) {
        Int num = std::stoll(words.at(i));

        if (num > declaredVarCount || num < -declaredVarCount) {
          showError("literal '" + to_string(num) + "' inconsistent with declared var count '" + to_string(declaredVarCount) + "' | line " + to_string(lineIndex));
        }

        if (num == 0) {
          if (i != wordCount - 1) {
            showError("clause terminated prematurely by '0' | line " + to_string(lineIndex));
          }

          addClause(clause);
          processedClauseCount++;
        }
        else { // literal
          if (i == wordCount - 1) {
            showError("missing end-of-clause indicator '" + to_string(0) + "' | line " + to_string(lineIndex));
          }
          clause.push_back(num);
        }
      }
    }
  }

  if (filePath == STDIN_CONVENTION) {
    printComment("getting cnf from stdin: done");
    printThickLine();
  }

  if (problemLineIndex == DUMMY_MIN_INT) {
    showError("no problem line before cnf file ends on line " + to_string(lineIndex));
  }

  if (weightFormat != WeightFormat::WPCNF) {
    for (Int var = 1; var <= declaredVarCount; var++) {
      additiveVars.insert(var);
    }
  }

  switch (weightFormat) {
    case WeightFormat::MINIC2D: {
      if (minic2dWeightsLineIndex == DUMMY_MIN_INT) {
        showError("MINIC2D weights line not found");
      }
      break;
    }
    case WeightFormat::UNWEIGHTED: { // populates literalWeights with 1s
      for (Int var = 1; var <= declaredVarCount; var++) {
        literalWeights[var] = 1;
        literalWeights[-var] = 1;
      }
      break;
    }
    case WeightFormat::CACHET: { // completes literalWeights
      for (Int var = 1; var <= declaredVarCount; var++) {
        Float varWeight = 0.5; // default
        if (literalWeights.find(var) != literalWeights.end()) {
          varWeight = literalWeights.at(var);
        }

        Float negativeLiteralWeight = 1 - varWeight;
        if (varWeight == -1) {
          varWeight = negativeLiteralWeight = 1;
        }

        literalWeights[var] = varWeight;
        literalWeights[-var] = negativeLiteralWeight;
      }
      break;
    }
    default: { // completes literalWeights for WCNF and WPCNF
      for (Int var = 1; var <= declaredVarCount; var++) {
        if (literalWeights.find(var) == literalWeights.end()) {
          literalWeights[var] = 1;
        }
        if (literalWeights.find(-var) == literalWeights.end()) {
          literalWeights[-var] = 1;
        }
      }
    }
  }

  checkLiteralWeights();

  if (verbosityLevel >= 1) {
    util::printRow("declaredVarCount", declaredVarCount);
    util::printRow("apparentVarCount", apparentVars.size());
    util::printRow("declaredClauseCount", declaredClauseCount);
    util::printRow("apparentClauseCount", processedClauseCount);
  }

  if (verbosityLevel >= 2) {
    printThinLine();
    printAdditiveVars();
    printLiteralWeights();
    printClauses();
    printThinLine();
  }
}
