#ifndef MMQuickEvent_H 
#define MMQuickEvent_H

#include "CCommonIncludes.h"
#include "CutStatistic.h"
#include "MapFile.h"

using namespace std;

// Mapping of APV-Chips
const int APVIDMM_X0 = 5;
const int APVIDMM_X1 = 4;
const int APVIDMM_X2 = 6;
const int APVIDMM_Y0 = 0;
const int APVIDMM_Y1 = 1;
const int APVIDMM_Y2 = 2;

//number of strips in x and y
const int xStrips = 360;
const int yStrips = 360;

const int NUMBER_OF_TIME_SLICES = 27;

template<class T1, class T2>
struct sort_pair_first {
	bool operator()(const std::pair<T1, T2>&left,
			const std::pair<T1, T2>&right) {
		return left.first < right.first;
	}
};

class MMQuickEvent {
public:
	MMQuickEvent(vector<string> vecFilenames, string Tree_Name,
			int NumberOfEvents = 10) {
		m_tchain = new TChain(Tree_Name.c_str());
		for (unsigned int i = 0; i < vecFilenames.size(); i++) {
			m_tchain->Add(vecFilenames[i].c_str());
			m_tchain->AddFriend("data", vecFilenames[i].c_str());
		}

		cleanVariables();
		addBranches();
		m_actEventNumber = 0;
		m_NumberOfEvents = m_tchain->GetEntries();
		if (NumberOfEvents != -1)
			m_NumberOfEvents = NumberOfEvents;
		cout << "[MMQuickEvent] Number of Events loaded: " << m_NumberOfEvents
				<< endl;

		maxChargeX = 0;
		stripWithMaxChargeX = 0;
		timeSliceOfMaxChargeX = 0;
		maxChargeY = 0;
		stripWithMaxChargeY = 0;
		timeSliceOfMaxChargeY = 0;

		numberOfXHits = 0;
		numberOfYHits = 0;
	}

	bool getNextEvent() {
		if (m_actEventNumber >= m_NumberOfEvents) {
			cout << endl;
			return false;
		}
		if (m_actEventNumber == 0)
			cout << "[MMQuickEvent] Looping over Events" << endl;
		if (m_actEventNumber % (m_NumberOfEvents / 100) == 0) {
			cout << '\r' << "[MMQuickEvent] "
					<< TMath::Nint(
							m_actEventNumber / ((float) m_NumberOfEvents)
									* 100.) << "% done (Event "
					<< m_actEventNumber << "/" << m_NumberOfEvents << ")...";
			cout.flush();
		} else if (m_actEventNumber == m_NumberOfEvents - 1) {
			cout << '\r' << "[MMQuickEvent] Done!                              "
					<< std::endl;
			cout.flush();
		}
		m_tchain->GetEvent(m_actEventNumber);
		m_actEventNumber++;

		return true;
	}

	void cleanVariables() {
		apv_fecNo = 0;
		apv_id = 0;
		apv_ch = 0;
		mm_id = 0;
		mm_readout = 0;
		mm_strip = 0;
		apv_q = 0;

		apv_qmax = 0;
		apv_tbqmax = 0;
	}

	int getEventNumber() {
		return m_NumberOfEvents;
	}

	int getCurrentEventNumber() {
		return m_actEventNumber;
	}

	void addBranches() {
		m_tchain->SetBranchAddress("apv_evt", &apv_evt);
		m_tchain->SetBranchAddress("time_s", &time_s);
		m_tchain->SetBranchAddress("time_us", &time_us);
		m_tchain->SetBranchAddress("apv_fecNo", &apv_fecNo);
		m_tchain->SetBranchAddress("apv_id", &apv_id);
		m_tchain->SetBranchAddress("apv_ch", &apv_ch);
		m_tchain->SetBranchAddress("mm_id", &mm_id);
		m_tchain->SetBranchAddress("mm_readout", &mm_readout);
		m_tchain->SetBranchAddress("mm_strip", &mm_strip);
		m_tchain->SetBranchAddress("apv_q", &apv_q);
		m_tchain->SetBranchAddress("apv_presamples", &apv_presamples);

		m_tchain->SetBranchAddress("apv_qmax", &apv_qmax);
		m_tchain->SetBranchAddress("apv_tbqmax", &apv_tbqmax);
	}

	// functions to select if hit is in X or Y according to APV ID and mapping while data acquisition
	static bool isX(int id) {
		if (id == APVIDMM_X0 || id == APVIDMM_X1 || id == APVIDMM_X2) {
			return true;
		} else {
			return false;
		}
	}

	static bool isY(int id) {
		if (id == APVIDMM_Y0 || id == APVIDMM_Y1 || id == APVIDMM_Y2) {
			return true;
		} else {
			return false;
		}
	}

public:
	TChain *m_tchain;
	int m_actEventNumber;
	int m_NumberOfEvents;

	/// Event Information
	// Declaration of leaf types
	UInt_t apv_evt;
	Int_t time_s;
	Int_t time_us;
	vector<unsigned int> *apv_fecNo;
	vector<unsigned int> *apv_id;
	vector<unsigned int> *apv_ch;
	vector<string> *mm_id;
	vector<unsigned int> *mm_readout;
	vector<unsigned int> *mm_strip;
	vector<vector<short> > *apv_q;
	UInt_t apv_presamples;

	vector<short> *apv_qmax;
	vector<short> *apv_tbqmax;

	short maxChargeX;
	int stripWithMaxChargeX;
	int timeSliceOfMaxChargeX;
	short maxChargeY;
	int stripWithMaxChargeY;
	int timeSliceOfMaxChargeY;

	short numberOfXHits;
	short numberOfYHits;

	vector<std::pair<int, short> > stripAndChargeAtMaxChargeTimeX; // absolute strip number and charges of all strips at fixed time section (being the maximum charge time)
	vector<std::pair<int, short> > stripAndChargeAtMaxChargeTimeY;

	int positionOfMaxChargeInCrossSectionX;
	int positionOfMaxChargeInCrossSectionY;

	/**
	 * Returns true if the neighbour strips of the strip with maximum charge are within a given range
	 */
	void generateFixedTimeCrossSections() {
		/*
		 * Store the charge values of every strip number for the time section with
		 * the maximum charge found in one event for X and Y separately (cross section
		 * for time sections with max charge)
		 */
		stripAndChargeAtMaxChargeTimeX.clear();
		stripAndChargeAtMaxChargeTimeY.clear();

		const unsigned int numberOfStrips = (*apv_q).size();
		// Iterate through all strips
		for (unsigned int strip = 0; strip != numberOfStrips; strip++) {
			unsigned int apvID = (*apv_id)[strip];
			if (MMQuickEvent::isX(apvID)) { // X axis
				stripAndChargeAtMaxChargeTimeX.push_back(
						std::make_pair((*mm_strip)[strip]/*Strip number*/,
								(*apv_q)[strip][timeSliceOfMaxChargeX]/*Charge*/));
			} else { // Y axis
				stripAndChargeAtMaxChargeTimeY.push_back(
						std::make_pair((*mm_strip)[strip]/*Strip number*/,
								(*apv_q)[strip][timeSliceOfMaxChargeY]/*Charge*/));
			}
		}

		// Sort cross section by absolute strip numbers (first entry in pairs)
		std::sort(stripAndChargeAtMaxChargeTimeX.begin(),
				stripAndChargeAtMaxChargeTimeX.end(),
				sort_pair_first<unsigned int, short>());

		std::sort(stripAndChargeAtMaxChargeTimeY.begin(),
				stripAndChargeAtMaxChargeTimeY.end(),
				sort_pair_first<unsigned int, short>());
		/*
		 * Now the array is sorted, the position of the maximal charge strip is unknown -> search for it again
		 */
		const unsigned int numberOfXStrips =
				stripAndChargeAtMaxChargeTimeX.size();
		for (positionOfMaxChargeInCrossSectionX = 0;
				positionOfMaxChargeInCrossSectionX != numberOfXStrips;
				positionOfMaxChargeInCrossSectionX++) {
			if (stripAndChargeAtMaxChargeTimeX[positionOfMaxChargeInCrossSectionX].second
					== maxChargeX) {
				break;
			}
		}

		const unsigned int numberOfYStrips =
				stripAndChargeAtMaxChargeTimeY.size();
		for (positionOfMaxChargeInCrossSectionY = 0;
				positionOfMaxChargeInCrossSectionY != numberOfYStrips;
				positionOfMaxChargeInCrossSectionY++) {
			if (stripAndChargeAtMaxChargeTimeY[positionOfMaxChargeInCrossSectionY].second
					== maxChargeY) {
				break;
			}
		}
	}

	void generateTimeShape(TH2F* histo, short maxCharge, int stripWithMaxCharge,
			int timeSliceOfMaxCharge) {
		/*
		 * Store the charge values of every strip number for the time section with
		 * the maximum charge found in one event for X and Y separately (cross section
		 * for time sections with max charge)
		 */
		const unsigned int numberOfTimeSlices = (*apv_q)[0].size();
		// Iterate through all strips
		for (unsigned int time = 0; time != numberOfTimeSlices; time++) {
			int distanceToMax = time - timeSliceOfMaxCharge;
			if (distanceToMax != 0) {
				double chargeProportion = 100
									* (double) (*apv_q)[stripWithMaxCharge][time] / maxCharge;
				histo->Fill(distanceToMax, chargeProportion);
			}
		}
	}

	bool runProportionCut(TH2F* maxNeighbourHisto,
			vector<std::pair<int, short> > stripAndChargeAtMaxChargeTime,
			short maxCharge, std::vector<std::pair<int, int> > proportionLimits,
			CutStatistic& absolutePositionCuts, CutStatistic& proportionCuts,
			bool lastProportionCut, int positionOfMaxCharge) {

		if (stripAndChargeAtMaxChargeTime.size() == 0) {
			return false;
		}

		/*
		 * Start at strip N left to the maximum charge strip where N is the number of entries in the propotion limits file
		 */
		bool absolutePositionCut = false;
		bool proportionCut = false;

		const int maxDistance = proportionLimits.size();
		for (int deltaStrip = -maxDistance; deltaStrip <= maxDistance;
				deltaStrip++) {
			if (deltaStrip == 0) {
				// don't store the maximum strip itself
				continue;
			}

			// Look at the x/y strips deltaStrip away from the maximal charge strip in x/y
			int stripIndexInCrossSection = positionOfMaxCharge + deltaStrip;

			// Check if all neighbour strips are available
			bool tooFarToTheLeft =
					stripAndChargeAtMaxChargeTime[positionOfMaxCharge].first
							+ deltaStrip <= 0; // absolute strip too far to the left
			bool tooFarToTheRight =
					stripAndChargeAtMaxChargeTime[positionOfMaxCharge].first
							+ deltaStrip > xStrips; // absolute strip too far to the left

			bool lowerLimitIsLargerZero =
					proportionLimits[abs(deltaStrip) - 1].first > 0; // only check if strip has charge stored if lower limit is larger zero

			bool stripChargeIsStored =
					(stripIndexInCrossSection >= 0 // too far to the left in the array?
							&& stripIndexInCrossSection
									< stripAndChargeAtMaxChargeTime.size() // too far to the right in the array
							&& stripAndChargeAtMaxChargeTime[stripIndexInCrossSection].first // check if there is a jump in the array
									== stripAndChargeAtMaxChargeTime[positionOfMaxCharge].first
											+ deltaStrip);

			double proportion = NAN;
			if (tooFarToTheRight || tooFarToTheLeft) {
				absolutePositionCut = true;
			} else if (!stripChargeIsStored && lowerLimitIsLargerZero) {
				proportionCut = true;
			} else {
				if (stripChargeIsStored) {
					proportion =
							100
									* stripAndChargeAtMaxChargeTime[stripIndexInCrossSection].second
									/ (double) maxCharge;

					// Cut for neighbours of maximum bin
					if (proportion < proportionLimits[abs(deltaStrip) - 1].first
							|| proportion
									> proportionLimits[abs(deltaStrip) - 1].second) {
						proportionCut = true;
					}
				}
			}

			if (proportion != NAN) {
				// Fill histogramm mmhitneighboursX and mmhitneighboursY
				maxNeighbourHisto->Fill((deltaStrip), proportion);
			}
		}

		if (!lastProportionCut) {
			if (absolutePositionCut) {
				absolutePositionCuts.Fill(1, this);
			} else {
				absolutePositionCuts.Fill(0, this);

				if (proportionCut) {
					proportionCuts.Fill(1, this);
				} else {
					proportionCuts.Fill(0, this);
				}
			}
		}

		return !absolutePositionCut && !proportionCut;
	}

	int calculateClusterSize(
			vector<std::pair<unsigned int, short> > stripAndChargeAtMaxChargeTime,
			int positionOfMaxCharge) {

		int clusterStart = 0;
		int clusterEnd = 0;
		int delta = 1;

		while (true) {
			bool finish = true;
			if (positionOfMaxCharge + delta
					< stripAndChargeAtMaxChargeTime.size()) {
				if (stripAndChargeAtMaxChargeTime[positionOfMaxCharge + delta].first
						== stripAndChargeAtMaxChargeTime[positionOfMaxCharge].first
								+ delta) {
					clusterEnd = delta;
					finish = false;
				}
			}

			if (positionOfMaxCharge - delta != -1) {
				if (stripAndChargeAtMaxChargeTime[positionOfMaxCharge - delta].first
						== stripAndChargeAtMaxChargeTime[positionOfMaxCharge].first
								- delta) {
					clusterStart = -delta;
					finish = false;
				}
			}
			delta++;
			if (finish) {
				break;
			}
		}
		return clusterEnd - clusterStart + 1;
	}

	void findMaxCharge() {

		vector<unsigned int> apvIDofStrip = *apv_id; // isX(apvIDofStrip[i]) returns true if the i-th strip is X-layer
		vector<unsigned int> stripNumShowingSignal = *mm_strip; // stripNumShowingSignal[i] is absolute strip number (strips without charge are not stored anywhere)
		vector<vector<short> > chargeOfStripOfTime = *apv_q; // chargeOfStripOfTime[i][j] is the charge of strip i in time section j (matrix of whole event)
		vector<short> maxChargeOfStrip = *apv_qmax; // maxChargeOfStrip[i] is the maxmimal measured charge of strip i of all time sections
		vector<short> timeSliceOfMaxChargeOfStrip = *apv_tbqmax; // timeSliceOfMaxChargeOfStrip[i] is the time section of the corresponding maximum charge (see above)

		maxChargeX = -1;
		stripWithMaxChargeX = -1;
		timeSliceOfMaxChargeX = -1;
		maxChargeY = -1;
		stripWithMaxChargeY = -1;
		timeSliceOfMaxChargeY = -1;

		numberOfXHits = -1;
		numberOfYHits = -1;

		/*
		 * Iterate through all strips and check if it is X or Y data. Compare the maximum charge
		 * of the strip with the maximum charge found so far for the current axis. Store current charge, strip number
		 * and time section with the maximum charge if the current charge is larger than before.
		 */
		const unsigned int numberOfStrips = maxChargeOfStrip.size();
		for (unsigned int strip = 0; strip != numberOfStrips; strip++) {
			unsigned int apvID = apvIDofStrip[strip];
			if (isX(apvID)) { // X axis
				numberOfXHits++;
				if (maxChargeOfStrip[strip] > maxChargeX) {
					maxChargeX = maxChargeOfStrip[strip];
					stripWithMaxChargeX = strip;
					timeSliceOfMaxChargeX = timeSliceOfMaxChargeOfStrip[strip];
				}
			} else { // Y axis
				numberOfYHits++;
				if (maxChargeOfStrip[strip] > maxChargeY) {
					maxChargeY = maxChargeOfStrip[strip];
					stripWithMaxChargeY = strip;
					timeSliceOfMaxChargeY = timeSliceOfMaxChargeOfStrip[strip];
				}
			}
		}
	}

	/**
	 * Generates a new 2D histogram (heatmap) showing all measured charges in all strips of x or y direction of all times sections.
	 * The histogram will be stored at general_mapHist2DEvent[eventNumber+"nameOfHistogram"]
	 */
	void generateEventDisplay(TH2F* &eventDisplayX, TH2F* &eventDisplayY,
			std::string suffix = "") {
		vector<vector<short> > chargeOfTimeOfStrip = *apv_q;
		unsigned int numberOfTimeSlices = chargeOfTimeOfStrip[0].size();

		/*
		 * Generate a new 2D histogram for the event display (x=strip, y=timesection, z=charge)
		 */

		// Generate the title of the histogram
		stringstream histoName;
		histoName.str("");
		histoName << getCurrentEventNumber() << "-Eventdisplay";

		string histoNameX = histoName.str() + "_X" + suffix;
		string histoNameY = histoName.str() + "_Y" + suffix;

		/*
		 * Initialize a new root TH2F histogram with the right title, labels and bining:
		 *
		 */
		eventDisplayX = new TH2F(histoNameX.c_str(),
				";strip number; time [25 ns]; charge", xStrips, 0, xStrips - 1,
				numberOfTimeSlices, 0, numberOfTimeSlices - 1);

		eventDisplayY = new TH2F(histoNameY.c_str(),
				";strip number; time [25 ns]; charge", yStrips, 0, yStrips - 1,
				numberOfTimeSlices, 0, numberOfTimeSlices - 1);

		/*
		 * Fill eventDisplay with all measured charges at all strips for all times;
		 * for all timeSlices, we iterate through all Strips (X and Y). Depending
		 * on the apvID, the corresponding histogram is filled (X resp. Y).
		 */
		for (unsigned int timeSlice = 0; timeSlice != numberOfTimeSlices;
				timeSlice++) {
			for (unsigned int stripNum = 0;
					stripNum != chargeOfTimeOfStrip.size(); stripNum++) {
				short charge = chargeOfTimeOfStrip[stripNum][timeSlice];
				unsigned int apvID = (*apv_id)[stripNum];
				if (MMQuickEvent::isX(apvID)) {

					// store charge of current strip in bin corresponding to absolute strip number
					eventDisplayX->SetBinContent(
							mm_strip->at(stripNum) + 1/*x*/, timeSlice + 1/*y*/,
							charge/*z*/);
				} else {
					eventDisplayY->SetBinContent(
							mm_strip->at(stripNum) + 1/*x*/, timeSlice + 1/*y*/,
							charge/*z*/);
				}
			}
		}
	}
}
;

#endif
