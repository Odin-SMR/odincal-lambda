function [data] = getdata(S)
% GETDATA get spectral data
%    [D] = GETDATA(S) returns the data of all spectra from an array
%    of Odin spectra. Data are returned as a matrix where each column
%    is a data vector of one spectrum in S
  
  m = length(S);
  n = length(S(1).data);
  data = zeros(n,m);
  for i = 1:m
    k = length(S(i).data);
    if (k ~= n)
      w = sprintf('no. of channel mismatch [%d]: %d (%d)', i, k, n);
      k = min([n k]);
      warning(w)
    end
    data(1:k,i) = S(i).data(1:k);
  end
