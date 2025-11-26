#include <iostream>
#include <fstream>
using namespace std;

void highScore(int score)
{
    int oldHighScore = 0;

    ifstream infile("Score.txt");
    if (infile.is_open())
    {
        infile >> oldHighScore;
        infile.close();
    }

    int newHighScore; 
    if (score > oldHighScore)
    {
        newHighScore = score;
    }
    else
        newHighScore = oldHighScore; 

    ofstream outfile("Score.txt", ios::trunc);
    outfile << newHighScore;
    outfile.close();

    cout << "High Score Updated: " << newHighScore << endl;
}

