#include "test_pooling_common.hpp"
#include "test_convolution_common.hpp"

hipdnnPoolingMode_t poolCPI_mode;

TEST(convolution_pooling_fwd_bwd_intg, func_check_naive_conv_pool_fwd_bwd) {

  Desc inputDescP(1, 1, 4, 4);
  Desc inputDesc(1, 1, 16, 16);
  Desc filterDesc(1, 1, 4, 4);

  int pad[2] = {0, 0};    // zero padding
  int stride[2] = {4, 4}; // stride 1
  int dil[2] = {1,1};
  int spatial_ext[2] = {2, 2};
  int strideP[2] = {2, 2};
  int pad_p[2] = {0,0};
  float avg_time = 0, avg_time1 = 0, avg_time2 = 0, avg_time3 = 0, avg_time4 = 0;

  alpha = 1.f;
  beta = 0.f;

  poolCPI_mode = HIPDNN_POOLING_MAX;
  hipdnnDataType_t dataType = HIPDNN_DATA_FLOAT;

  Desc outputDesc = calculate_Dims(inputDesc, filterDesc, pad, stride, dil);
  Desc outputDescP = calculate_pool_Dims(inputDescP, spatial_ext, pad_p, strideP);

  activation_params_t test_case1(1, 1, 4, 4);
  convulution_Size testConvolutionSizes(
        inputDesc.N, 1, inputDesc.C, inputDesc.H, inputDesc.W, outputDesc.C,
        outputDesc.H, outputDesc.W, filterDesc.H, filterDesc.W, pad[0], pad[1],
        stride[0], stride[1], dil[0], dil[1]);

  convulution_Size testConvolutionSizes2(
        test_case1.n, 1, test_case1.channels, test_case1.height, test_case1.width,
        test_case1.channels, test_case1.height, test_case1.width, filterDesc.H,
        filterDesc.W, pad[0], pad[1], stride[0], stride[1], dil[0], dil[1]);

  test_pooling_descriptor pool(inputDescP.N, inputDescP.C, inputDescP.H,
                               inputDescP.W, outputDescP.H, outputDescP.W,
                               spatial_ext[0], spatial_ext[1], pad_p[0],
                               pad_p[1], strideP[0], strideP[1]);

  Memory<float> srcDataConv = createMemory<float>(inputDesc);
  Memory<float> dstDataGPU = createMemory<float>(outputDesc);
  Memory<float> filterData = createMemory<float>(filterDesc);
  Memory<float> gradData1 = createMemory<float>(outputDesc);
  Memory<float> gradData2 = createMemory<float>(filterDesc);
  Memory<float> dstData = createMemory<float>(outputDescP);
  Memory<float> dataSrc_act(test_case1.n * test_case1.channels * test_case1.height *
                        test_case1.width);
  Memory<float> dataGrad_act(test_case1.n * test_case1.channels * test_case1.height
                          * test_case1.width);
  Memory<float> dataDst_act(test_case1.n * test_case1.channels * test_case1.height *
                        test_case1.width);

  populateMemoryRandom<float>(srcDataConv);
  populateMemoryRandom<float>(filterData);
  populateMemoryRandom(dataSrc_act);

  int ip_size_cf[4] = {inputDesc.N, inputDesc.C, inputDesc.H, inputDesc.W};
  int k_size_cf[4] = {filterDesc.N, filterDesc.C, filterDesc.H, filterDesc.W};
  int op_size_cf[4] =  {outputDesc.N, outputDesc.C, outputDesc.H, outputDesc.W};

  int ip_size_pf[4] = {outputDesc.N, outputDesc.C, outputDesc.H, outputDesc.W};
  int k_size_pf[4] = {pool.mb, pool.c, pool.kh, pool.kw};
  int op_size_pf[4] =  {pool.mb, pool.c, pool.oh, pool.ow};

  int ip_size_pb[4] = {pool.mb, pool.c, pool.oh, pool.ow};
  int k_size_pb[4] = {pool.mb, pool.c, pool.kh, pool.kw};
  int op_size_pb[4] =  {outputDesc.N, outputDesc.C, outputDesc.H, outputDesc.W};

  int ip_size_cb[4] = {outputDesc.N, outputDesc.C, outputDesc.H, outputDesc.W};
  int k_size_cb[4] = {filterDesc.N, filterDesc.C, filterDesc.H, filterDesc.W};
  int op_size_cb[4] =  {filterDesc.N, filterDesc.C, filterDesc.H, filterDesc.W};

  std::string str_ip_size = integration_dims_to_string2(ip_size_cf,ip_size_pf,
                                      ip_size_cb,ip_size_pb,"Conv_fwd","MP_fwd",
                                      "Conv_bwd","MP_bwd");

  std::string str_k_size = integration_dims_to_string2(k_size_cf,k_size_pf,
                                      k_size_cb,k_size_pb,"Conv_fwd","MP_fwd",
                                      "Conv_bwd","MP_bwd");

  std::string str_op_size = integration_dims_to_string2(op_size_cf,op_size_pf,
                                      op_size_cb,op_size_pb, "Conv_fwd","MP_fwd",
                                      "Conv_bwd","MP_bwd");

  compute_hipdnn_conv_forward<float>(testConvolutionSizes, srcDataConv.gpu(),
                          filterData.gpu(), NULL, dstDataGPU.gpu(), dataType, alpha, beta,
                          &avg_time1);

  hipdnn_pooling_backward<float>(pool, dstDataGPU.gpu(), gradData1.gpu(),
                dstData.gpu(), poolCPI_mode, dataType, alpha, beta, &avg_time3);

  compute_hipdnn_conv_backward_filter<float>(testConvolutionSizes2, dstDataGPU.gpu(),
                                 filterData.gpu(), gradData2.gpu(), NULL,
                                 gradData1.gpu(), alpha, beta, &avg_time4);

  avg_time = avg_time1 + avg_time2 + avg_time3 + avg_time4;

  std::cout << "\nAverage Time is: " << avg_time << "micro seconds"<<std::endl;

  std::string strt = "./result_unittest.csv";
  std::string testname =
         "convolution_pooling_fwd_bwd_intg: func_check_naive_conv_pool_fwd_bwd";
  std::string filename = "convolution_pooling_fwd_bwd_intg.csv";

  float* temp = gradData2.getDataFromGPU();

  std::string str  = convert_to_string((float*)temp,(int)gradData2.get_num_elements());

  write_to_csv(strt, str, testname, avg_time, str_ip_size, str_k_size, str_op_size);
  dump_result_csv(filename, testname, temp, (int)gradData2.get_num_elements());
}