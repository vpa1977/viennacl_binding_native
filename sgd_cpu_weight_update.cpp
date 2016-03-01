#include <jni.h>
#include "stdafx.h"
#include "viennacl-java-binding.h"
#include <viennacl/forwards.h>
#include <viennacl/context.hpp>
#include <viennacl/abstract_kernel.hpp>
#include <stdint.h>
#include <atomic>



#include "jni_setup.h"
#include "org_viennacl_binding_Context.h"
#include "org_viennacl_binding_Buffer.h"
#include "jni_viennacl_context.h"
#include "org_moa_opencl_sgd_DirectUpdater.h"
#include "org_moa_opencl_sgd_OneBitUpdater.h"

JNIEXPORT void JNICALL Java_org_moa_opencl_sgd_DirectUpdater_nativeAtomicUpdate
(JNIEnv * env, jobject obj, jlong gradient, jlong weight, jint count, jdouble learning_rate)
{
	std::_Atomic_impl<8U> func;
	double * gradient_ptr = (double*)gradient;
	double*  weight_ptr = (double*)weight;
	for (int i = 0; i < count; ++i)
	{
		double temp; 
		if (gradient_ptr[i] != 0.0)
		{
			func._Load(&temp, &weight_ptr[i], std::memory_order_acquire);
			temp += gradient_ptr[i] *learning_rate;
			func._Store(&weight_ptr[i], &temp, std::memory_order_release);
			//weight_ptr[i] += gradient_ptr[i] * learning_rate;
		}
	}
}


/*
* Class:     org_moa_opencl_sgd_OneBitUpdater
* Method:    nativeComputeUpdate
* Signature: (JJJJJIIDI)V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_sgd_OneBitUpdater_nativeComputeUpdate
(JNIEnv * env, jobject obj, 
	jlong g, jlong w, jlong wd, jlong el, jlong es, jlong tau, 
	jint num_attributes, jint num_classes, jdouble learning_rate, jint worker_id)
{
	NUM_DATA_TYPE * Es = (NUM_DATA_TYPE*)es;
	NUM_DATA_TYPE * El = (NUM_DATA_TYPE*)el;
	NUM_DATA_TYPE* gradients = (NUM_DATA_TYPE*)g;
	long * weights_delta = (long*)wd;
	NUM_DATA_TYPE* Tau = (NUM_DATA_TYPE*)tau;
	
	for (int row = 0; row < num_classes; ++row)
	{
		int error_offset = worker_id*num_classes *num_attributes + num_attributes*row;
		for (int column = 0; column < num_attributes; ++column)
		{
			NUM_DATA_TYPE gradient = gradients[row *num_attributes + column] + Es[error_offset + column] + El[error_offset + column];
			NUM_DATA_TYPE abs_grad = fabs(gradient);
			if (gradient == 0)
				continue;
			if (abs_grad + 0.000000000001 >= Tau[column + row*num_classes])
			{
				long upd = 1;
				if (gradient < 0)
					upd = -1;
				InterlockedAdd(&weights_delta[row * num_attributes + column], upd);
				abs_grad -= Tau[column];
				El[error_offset + column] = ((gradient > 0) - (gradient < 0))*abs_grad;
				Es[error_offset + column] = 0;
			}
			else
			{
				El[error_offset + column] = 0;
				Es[error_offset + column] = gradient;
			}
		}

	}
}

//private native void nativeComputeWeight(long weight, long tau, long delta, double rate);
/*
* Class:     org_moa_opencl_sgd_OneBitUpdater
* Method:    nativeComputeWeight
* Signature: (JJJD)V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_sgd_OneBitUpdater_nativeComputeWeight
(JNIEnv * env, jobject obj, jlong w, jlong tau, jlong delta, jdouble rate, jint num_attributes, jint num_classes)
{
	NUM_DATA_TYPE* weights = (NUM_DATA_TYPE*)w;
	NUM_DATA_TYPE* t = (NUM_DATA_TYPE*)tau;
	long* d = (long*)delta;

	for (int i = 0; i < num_attributes * num_classes; ++i)
		weights[i] += rate* t[i] *d[i];
}


/*
* Class:     org_moa_opencl_sgd_OneBitUpdater
* Method:    nativeUpdateTau
* Signature: (JJJJJIIII)V
*/
JNIEXPORT void JNICALL Java_org_moa_opencl_sgd_OneBitUpdater_nativeUpdateTau
(JNIEnv *, jobject, jlong tau, jlong es_avg, jlong el_avg, jlong es, jlong el, jint total_workers,
	jint num_attributes, jint num_classes, jint update_step)
{
	NUM_DATA_TYPE * Es = (NUM_DATA_TYPE*)es;
	NUM_DATA_TYPE * El = (NUM_DATA_TYPE*)el;
	NUM_DATA_TYPE* EsAvg = (NUM_DATA_TYPE*)es_avg;
	NUM_DATA_TYPE* ElAvg = (NUM_DATA_TYPE*)el_avg;
	NUM_DATA_TYPE* Tau = (NUM_DATA_TYPE*)tau;
	for (int j = 0; j < num_attributes * num_classes; ++j)
	{
		NUM_DATA_TYPE sumL = 0;
		NUM_DATA_TYPE sumR = 0;
		for (int worker = 0; worker < total_workers; ++worker)
		{
			sumL += fabs(Es[worker * num_attributes*num_classes + j]);
			sumR += Tau[j] + fabs(El[worker*num_attributes*num_classes + j]);
		}
		sumL = sumL / total_workers;
		sumR = sumR / total_workers;
		EsAvg[j] = ((update_step - 1) * EsAvg[j] + sumL) / (update_step );
		ElAvg[j] = ((update_step - 1) * ElAvg[j] + sumR) / (update_step);

		Tau[j] = (sumR - sumL) / 2;
	}
}
