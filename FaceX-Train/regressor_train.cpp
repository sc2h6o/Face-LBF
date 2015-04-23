/*
FaceX-Train is a tool to train model file for FaceX, which is an open
source face alignment library.

Copyright(C) 2014  Yang Cao

This program is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

#include "regressor_train.h"

#include <utility>
#include <iostream>
#include <memory>
#include <algorithm>

#include "utils_train.h"


using namespace std;

RegressorTrain::RegressorTrain(const TrainingParameters &tp)
	: training_parameters(tp)
{
	forests = vector<RFSTrain>(training_parameters.landmark_count, RFSTrain(tp));
}

void RegressorTrain::Regress(int index_reg, const vector<cv::Point2d> &mean_shape,
	vector<vector<cv::Point2d>> *targets,
	const vector<DataPoint> & training_data)
{
	for (int i = 0; i < training_parameters.landmark_count; ++i)
		forests[i].Regress(i, index_reg, mean_shape, targets, training_data);

	int num_leaves = pow(2, training_parameters.depth_trees - 1);
	int feat_length = training_parameters.landmark_count *
		training_parameters.num_trees * num_leaves;
	bool** bin_feats = new bool*[training_data.size()];
	for (int i = 0; i < training_data.size(); ++i)
	{
		bin_feats[i] = new bool[feat_length];
		for (int j = 0; j < training_parameters.landmark_count; j++)
		{
			forests[j].Apply(j, mean_shape, training_data[i], bin_feats[i]);
		}
	}
	cv::Mat mat_feats(training_data.size(), feat_length, IPL_DEPTH_1U, bin_feats);
	GlobalRegress(targets, mat_feats);
}

void RegressorTrain::GlobalRegress(std::vector<std::vector<cv::Point2d>> *targets,
	const cv::Mat &mat_feats)
{
	cv::SVMParams svm_param;
	svm_param.svm_type = CvSVM::EPS_SVR;
	svm_param.kernel_type = CvSVM::LINEAR;
	svm_param.p = 5e-3;
	svm_param.term_crit = cvTermCriteria(CV_TERMCRIT_EPS, 100, 5e-3);

	for (int i = 0; i < training_parameters.landmark_count; i++)
	{
		cv::Mat target_single_x = cv::Mat(mat_feats.rows, 1, CV_64FC1);
		cv::Mat target_single_y = cv::Mat(mat_feats.rows, 1, CV_64FC1);
		for (int j = 0; j < mat_feats.rows; j++)
		{
			target_single_x.at<double>(j, 0) = (*targets)[j][i].x;
			target_single_y.at<double>(j, 0) = (*targets)[j][i].y;
		}
	}
}

vector<cv::Point2d> RegressorTrain::Apply(const vector<cv::Point2d> &mean_shape, 
	const DataPoint &data) const
{
	int num_leaves = pow(2, training_parameters.depth_trees-1);
	vector<bool> bin_feat(training_parameters.landmark_count * 
		training_parameters.num_trees * num_leaves);
	for (int i = 0; i < training_parameters.landmark_count; ++i){
		forests[i].Apply(i, mean_shape, data, bin_feat);
	}
}

template<typename T1, typename T2>
void write(cv::FileStorage& fs, const string&, const std::pair<T1, T2>& p)
{
	fs << "{";
	cv::write(fs, "first", p.first);
	cv::write(fs, "second", p.second);
	fs << "}";
}

void RegressorTrain::write(cv::FileStorage &fs)const
{
	cv::write(fs, "forests", forests);
}

void write(cv::FileStorage& fs, const string&, const RegressorTrain& r)
{
	r.write(fs);
}