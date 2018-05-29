/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace utilities {

/**
 * @brief Return values in DataRange corresponding to matching Markers
 *
 * Takes two parallel ranges, a Data range containing data values, and a Marker range containing markers on the
 * corresponding data values. Returns a new Data range containing only the values corresponding to markers which match
 * markerValue
 *
 * For example:
 * @code{.cpp}
 * vector<char> data = {'A', 'B', 'C'};
 * vector<bool> markers = {true, false, true};
 * auto markedData = filter_data_by_marker(data, markers, true);
 * // markedData contains {'A', 'C'}
 * @endcode
 */
template<typename DataRange, typename MarkerRange, typename Marker>
DataRange
filter_data_by_marker(DataRange data, MarkerRange markers, const Marker value) {
    auto size = data.size();
    FC_ASSERT(size == markers.size(), "The size of data and markers should be match");

    auto itd = std::cbegin(data);
    auto itm = std::cbegin(markers);

    auto r = DataRange();
    r.reserve(size);

    for(uint i = 0; i < size; i++, itd++, itm++) {
        if(*itm == value) {
            r.emplace(*itd);
        }
    }
    return r;
}

}} // namespace evt::utilities
