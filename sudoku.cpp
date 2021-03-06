// Sudoku solver

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <bitset>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <set>
#include <cassert>
#include <numeric>

using namespace std;

#define F(i)  for(size_t i = 0; i < Cell::size(); ++i)


/**
 * Class to hold the value for a single cell in the puzzle
 * Very simple wrapper around bitset<> for readability
 */
class Cell {
    private:
        bitset<9> _bits;

    public:
        Cell() : _bits(0x1FF) {}

        size_t count() const
        {
            return _bits.count();
        }

        bool isSet(size_t i) const
        {
            return _bits[i];
        }

        void remove(size_t i)
        {
            _bits[i] = false;
        }

        size_t val() const
        {
            F(i)
                if (isSet(i))
                    return i;
            return size();
        }

        static size_t size()
        {
            return 9;
        }
};

class Board {
    private:
        vector<Cell> _cells;
        vector<vector<uint8_t>> group_counts;

        static vector<vector<size_t>> groups, belong_to;
        void initGroups();

        bool assign(size_t cell, size_t val);
        bool remove(size_t cell, size_t val);

        size_t smallest() const;

    public:

        explicit Board(const string& inp);

        bool solve();

        bool isSolved() const;
        bool isSolvedDebug() const;

        void printDebug() const;
};

vector<vector<size_t>> Board::groups, Board::belong_to(81);

void Board::initGroups()
{
    if (!groups.empty())
        return;

    // Create all the different groups as simple vector lists

    // Rows + columns
    for (size_t r = 0; r < 9; ++r) {
        vector<size_t> row, col;
        for (size_t c = 0; c < 9; ++c) {
            row.push_back(9*r + c);
            col.push_back(9*c + r);
        }
        groups.push_back(row);
        groups.push_back(col);
    }

    // Boxes
    for (size_t r = 0; r < 9; r+=3) {
        for (size_t c = 0; c < 9; c+=3) {
            vector<size_t> tmp;
            for (size_t rdiff = r; rdiff < r+3; ++rdiff)
                for (size_t cdiff = c; cdiff < c+3; ++cdiff)
                    tmp.push_back(9*rdiff + cdiff);
            groups.push_back(tmp);
        }
    }

    // Save which groups each cell belongs to
    for (size_t i = 0; i < groups.size(); ++i) {
        for (size_t g : groups[i])
            belong_to[g].push_back(i);
    }
}

Board::Board(const string& inp) : _cells(81)
{
    initGroups();
    group_counts.insert(group_counts.begin(), groups.size(), vector<uint8_t>{9,9,9,9,9,9,9,9,9});

    size_t i = 0;
    for (char c : inp) {
        if (c >= '1' && c <= '9') { // Make sure to convert to zero-indexed
            if (!assign(i++, static_cast<size_t>(c - '1'))) {
                return;
            }
        }
        if (c == '0' || c == '.')
            ++i;
    }
}

/**
 * Checks to see if the board is solved
 *
 * @return  True if solved
 */
bool Board::isSolved() const
{
    for (auto c : _cells)
        if (c.count() != 1)
            return false;
    return true;
}

/**
 * Checks to see if the board is solved according to Sudoku rules
 *
 * @return  True if solved
 */
bool Board::isSolvedDebug() const
{
    if (!isSolved())
        return false;
    for (auto& vec : groups) {
        // Check cells in vec for uniqueness
        set<size_t> check;
        for (size_t i : vec)
            check.insert(_cells[i].val());
        if (check.size() != 9)
            return false;
    }
    return true;
}

/**
 * Assigns a value to a cell
 *
 * @param   cell    Index of cell to assign the value
 * @param   val     Value to assign
 * @return  False if the assign is unsuccessful
 */
bool Board::assign(size_t cell, size_t val)
{
    assert( _cells[cell].isSet(val) );
    F(i) {
        if (i == val)
            continue;
        if (!remove(cell, i))
            return false;
    }
    return true;
}

/**
 * Removes the value from the given cell. Performs checks to see if other
 *  cells need to be updated
 *
 * @param   cell    Index of cell 
 * @param   val     Value to remove
 * @return  False if the remove is unsuccessful
 */
bool Board::remove(size_t cell, size_t val)
{
    auto& c = _cells[cell];
    if (!c.isSet(val))
        return true;
    c.remove(val);
    if (c.count() == 0)
        return false;
    if (c.count() == 1) {
        // This cell now has 1 value
        // Loop through every neighbor and remove that val
        // Keeping this here instead of in assign() removes code duplication

        size_t i = c.val();

        for (size_t n : belong_to[cell]) {
            for (size_t x : groups[n]) {
                if (x == cell)
                    continue;
                if (!remove(x, i))
                    return false;
            }
        }
    }

    // For every group that this cell is a part of, loop through each cell.
    // Checking to see if 'val' is unique in any group. If it is, assign it.
    for (size_t n : belong_to[cell]) {
        if (--group_counts[n][val] != 1)
            continue;

        // Find where that value is and set it
        for (size_t x : groups[n]) {
            if (!_cells[x].isSet(val))
                continue;
            if (!assign(x, val))
                return false;
            break;// Only one in group
        }
    }
    return true;
}

void Board::printDebug() const
{
    for (auto x : _cells) {
        if (x.count() == 1)
            cout << x.val()+1;
        else
            cout << static_cast<char>('A' + x.count());
    }
    cout << endl;
}

size_t Board::smallest() const
{
    size_t i;
    size_t min = SIZE_MAX;
    size_t mini = _cells.size(); // Intentionally invalid
    for (i = 0; i < _cells.size(); ++i) {
        if (_cells[i].count() == 1)
            continue;
        if (_cells[i].count() < min) {
            min = _cells[i].count();
            mini = i;
        }
    }
    return mini;
}

bool Board::solve()
{
    if (this->isSolved())
        return true;

    // Place a guess at the smallest unsolved place
    size_t i = this->smallest();
    //for (size_t j = 0; j < _cells[i].size(); ++j) {
    F(j) {
        if (!_cells[i].isSet(j))
            continue;

        // Create copy
        Board cop(*this);
        if (cop.assign(i, j)) {
            if (cop.solve()) {
                swap(*this, cop);
                return true;
            }
        }
    }
    return false;
}

int main()
{
    string line;
    size_t problems = 0;

    // Time everything
    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    vector<double> timings;

    duration<double> time_span;


    while (getline(cin, line)) {
        ++problems;

        // Start timer
        auto start_time = high_resolution_clock::now();

        // Create Board from line
        Board x(line);
        //solve(x);
        x.solve();

        // Stop timer
        auto end_time = high_resolution_clock::now();

        time_span = duration_cast<duration<double>>(end_time-start_time);
        timings.push_back(time_span.count());

        if (!x.isSolvedDebug()) {
            cout << "Impossible Problem." << endl;
            cout << "Took " << time_span.count() << " seconds." << endl;
        }
    }

    auto end = high_resolution_clock::now();
    time_span = duration_cast<duration<double>>(end-start);

    cout << "Solved " << problems << " Sudoku boards." << endl;
    cout << "Total Time: " << time_span.count() << endl;
    cout << "Solve Time: " << accumulate(timings.begin(), timings.end(), static_cast<double>(0)) << endl;
    cout << "Avg Time:   " << time_span.count() / problems << endl;
    cout << "Max Time:   " << *max_element(timings.begin(), timings.end()) << endl;
    cout << "Min Time:   " << *min_element(timings.begin(), timings.end()) << endl;

    return 0;
}
