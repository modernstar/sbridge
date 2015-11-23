/*
 * libpri: An implementation of Primary Rate ISDN
 *
 * Copyright (C) 2009 Digium, Inc.
 *
 * Richard Mudgett <rmudgett@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2 as published by the
 * Free Software Foundation. See the LICENSE file included with
 * this program for more details.
 *
 * In addition, when this program is distributed with Asterisk in
 * any form that would qualify as a 'combined work' or as a
 * 'derivative work' (but not mere aggregation), you can redistribute
 * and/or modify the combination under the terms of the license
 * provided with that copy of Asterisk, instead of the license
 * terms granted here.
 */

/*!
 * \file
 * \brief Remote Operations Service Element (ROSE) main controlling functions
 *
 * \author Richard Mudgett <rmudgett@digium.com>
 */


#include <stdio.h>

#include "compat.h"
#include "libpri.h"
#include "pri_internal.h"
#include "rose.h"
#include "rose_internal.h"
#include "asn1.h"
#include "pri_facility.h"


#define ROSE_TAG_COMPONENT_INVOKE	(ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_PC_CONSTRUCTED | 1)
#define ROSE_TAG_COMPONENT_RESULT	(ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_PC_CONSTRUCTED | 2)
#define ROSE_TAG_COMPONENT_ERROR	(ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_PC_CONSTRUCTED | 3)
#define ROSE_TAG_COMPONENT_REJECT	(ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_PC_CONSTRUCTED | 4)

/*! \brief Structure to convert a code value to a string */
struct rose_code_strings {
	/*! \brief Code value to convert to a string */
	int code;
	/*! \brief String equivalent of the associated code value */
	const char *name;
};

/*! \brief ROSE invoke/result message conversion table entry. */
struct rose_convert_msg {
	/*! \brief library encoded operation-value */
	enum rose_operation operation;
	/*!
	 * \brief OID prefix values to use when encoding/decoding the operation-value OID
	 * \note NULL if operation-value is a localValue.
	 */
	const struct asn1_oid *oid_prefix;
	/*! \brief Last OID value or localValue for the encoded operation-value */
	u_int16_t value;

	/*!
	 * \brief Encode the ROSE invoke operation-value arguments.
	 *
	 * \param ctrl D channel controller for diagnostic messages or global options.
	 * \param pos Starting position to encode ASN.1 component.
	 * \param end End of ASN.1 encoding data buffer.
	 * \param args Arguments to encode in the buffer.
	 *
	 * \retval Start of the next ASN.1 component to encode on success.
	 * \retval NULL on error.
	 *
	 * \note The function pointer is NULL if there are no arguments to encode.
	 */
	unsigned char *(*encode_invoke_args)(struct pri *ctrl, unsigned char *pos,
		unsigned char *end, const union rose_msg_invoke_args *args);
	/*!
	 * \brief Encode the ROSE result operation-value arguments.
	 *
	 * \param ctrl D channel controller for diagnostic messages or global options.
	 * \param pos Starting position to encode ASN.1 component.
	 * \param end End of ASN.1 encoding data buffer.
	 * \param args Arguments to encode in the buffer.
	 *
	 * \retval Start of the next ASN.1 component to encode on success.
	 * \retval NULL on error.
	 *
	 * \note The function pointer is NULL if there are no arguments to encode.
	 */
	unsigned char *(*encode_result_args)(struct pri *ctrl, unsigned char *pos,
		unsigned char *end, const union rose_msg_result_args *args);

	/*!
	 * \brief Decode the ROSE invoke operation-value arguments.
	 *
	 * \param ctrl D channel controller for diagnostic messages or global options.
	 * \param tag Component tag that identified this structure.
	 * \param pos Starting position of the ASN.1 component length.
	 * \param end End of ASN.1 decoding data buffer.
	 * \param args Arguments to fill in from the decoded buffer.
	 *
	 * \retval Start of the next ASN.1 component on success.
	 * \retval NULL on error.
	 *
	 * \note The function pointer is NULL if there are no arguments to decode.
	 */
	const unsigned char *(*decode_invoke_args)(struct pri *ctrl, unsigned tag,
		const unsigned char *pos, const unsigned char *end,
		union rose_msg_invoke_args *args);
	/*!
	 * \brief Decode the ROSE result operation-value arguments.
	 *
	 * \param ctrl D channel controller for diagnostic messages or global options.
	 * \param tag Component tag that identified this structure.
	 * \param pos Starting position of the ASN.1 component length.
	 * \param end End of ASN.1 decoding data buffer.
	 * \param args Arguments to fill in from the decoded buffer.
	 *
	 * \retval Start of the next ASN.1 component on success.
	 * \retval NULL on error.
	 *
	 * \note The function pointer is NULL if there are no arguments to decode.
	 */
	const unsigned char *(*decode_result_args)(struct pri *ctrl, unsigned tag,
		const unsigned char *pos, const unsigned char *end,
		union rose_msg_result_args *args);
};

/*! \brief ROSE error code conversion table entry. */
struct rose_convert_error {
	/*! \brief library encoded error-value */
	enum rose_error_code code;
	/*!
	 * \brief OID prefix values to use when encoding/decoding the error-value OID
	 * \note NULL if error-value is a localValue.
	 */
	const struct asn1_oid *oid_prefix;
	/*! \brief Last OID value or localValue for the encoded error-value */
	u_int16_t value;

	/*!
	 * \brief Encode the ROSE error parameters.
	 *
	 * \param ctrl D channel controller for diagnostic messages or global options.
	 * \param pos Starting position to encode ASN.1 component.
	 * \param end End of ASN.1 encoding data buffer.
	 * \param args Arguments to encode in the buffer.
	 *
	 * \retval Start of the next ASN.1 component to encode on success.
	 * \retval NULL on error.
	 *
	 * \note The function pointer is NULL if there are no arguments to encode.
	 */
	unsigned char *(*encode_error_args)(struct pri *ctrl, unsigned char *pos,
		unsigned char *end, const union rose_msg_error_args *args);

	/*!
	 * \brief Decode the ROSE error parameters.
	 *
	 * \param ctrl D channel controller for diagnostic messages or global options.
	 * \param tag Component tag that identified this structure.
	 * \param pos Starting position of the ASN.1 component length.
	 * \param end End of ASN.1 decoding data buffer.
	 * \param args Arguments to fill in from the decoded buffer.
	 *
	 * \retval Start of the next ASN.1 component on success.
	 * \retval NULL on error.
	 *
	 * \note The function pointer is NULL if there are no arguments to decode.
	 */
	const unsigned char *(*decode_error_args)(struct pri *ctrl, unsigned tag,
		const unsigned char *pos, const unsigned char *end,
		union rose_msg_error_args *args);
};


/* ------------------------------------------------------------------- */


/*
 * Note the first value in oid.values[] is really the first two
 * OID subidentifiers.  They are compressed using this formula:
 * First_Value = (First_Subidentifier * 40) + Second_Subidentifier
 */

/*! \brief ETSI specific invoke/result encode/decode message table */
static const struct rose_convert_msg rose_etsi_msgs[] = {
/* *INDENT-OFF* */
/*
 *		operation,                                  oid_prefix, value,
 *			encode_invoke_args,                     encode_result_args,
 *			decode_invoke_args,                     decode_result_args
 */
	/*
	 * localValue's from Advice-of-Charge-Operations
	 * {ccitt identified-organization etsi (0) 182 operations-and-errors (1)}
	 *
	 * Advice-Of-Charge-at-call-Setup(AOCS)
	 * Advice-Of-Charge-During-the-call(AOCD)
	 * Advice-Of-Charge-at-the-End-of-the-call(AOCE)
	 */
	{
		ROSE_ETSI_ChargingRequest,					NULL, 30,
			rose_enc_etsi_ChargingRequest_ARG,		rose_enc_etsi_ChargingRequest_RES,
			rose_dec_etsi_ChargingRequest_ARG,		rose_dec_etsi_ChargingRequest_RES
	},
	{
		ROSE_ETSI_AOCSCurrency,						NULL, 31,
			rose_enc_etsi_AOCSCurrency_ARG,			NULL,
			rose_dec_etsi_AOCSCurrency_ARG,			NULL
	},
	{
		ROSE_ETSI_AOCSSpecialArr,					NULL, 32,
			rose_enc_etsi_AOCSSpecialArr_ARG,		NULL,
			rose_dec_etsi_AOCSSpecialArr_ARG,		NULL
	},
	{
		ROSE_ETSI_AOCDCurrency,						NULL, 33,
			rose_enc_etsi_AOCDCurrency_ARG,			NULL,
			rose_dec_etsi_AOCDCurrency_ARG,			NULL
	},
	{
		ROSE_ETSI_AOCDChargingUnit,					NULL, 34,
			rose_enc_etsi_AOCDChargingUnit_ARG,		NULL,
			rose_dec_etsi_AOCDChargingUnit_ARG,		NULL
	},
	{
		ROSE_ETSI_AOCECurrency,						NULL, 35,
			rose_enc_etsi_AOCECurrency_ARG,			NULL,
			rose_dec_etsi_AOCECurrency_ARG,			NULL
	},
	{
		ROSE_ETSI_AOCEChargingUnit,					NULL, 36,
			rose_enc_etsi_AOCEChargingUnit_ARG,		NULL,
			rose_dec_etsi_AOCEChargingUnit_ARG,		NULL
	},
/* *INDENT-ON* */
};


/*! \brief ETSI specific error-value converion table */
static const struct rose_convert_error rose_etsi_errors[] = {
/* *INDENT-OFF* */
/*
 *		error-code,                                 oid_prefix, value
 *			encode_error_args,                      decode_error_args
 */
	/*
	 * localValue Errors from General-Errors
	 * {ccitt identified-organization etsi(0) 196 general-errors(2)}
	 */
	{
		ROSE_ERROR_Gen_NotSubscribed,				NULL, 0,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_NotAvailable,				NULL, 3,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_NotImplemented,				NULL, 4,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InvalidServedUserNr,			NULL, 6,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InvalidCallState,			NULL, 7,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_BasicServiceNotProvided,		NULL, 8,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_NotIncomingCall,				NULL, 9,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_SupplementaryServiceInteractionNotAllowed,NULL, 10,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_ResourceUnavailable,			NULL, 11,
			NULL,									NULL
	},

	/*
	 * localValue Errors from Advice-of-Charge-Operations
	 * {ccitt identified-organization etsi (0) 182 operations-and-errors (1)}
	 */
	{
		ROSE_ERROR_AOC_NoChargingInfoAvailable,		NULL, 26,
			NULL,									NULL
	},
/* *INDENT-ON* */
};


/* ------------------------------------------------------------------- */


/*! \brief Q.SIG specific invoke/result encode/decode message table */
static const struct rose_convert_msg rose_qsig_msgs[] = {
/* *INDENT-OFF* */
/*
 *		operation,                                  oid_prefix, value,
 *			encode_invoke_args,                     encode_result_args,
 *			decode_invoke_args,                     decode_result_args
 */
	/*
	 * localValue's from Q.SIG Name-Operations
	 * { iso(1) standard(0) pss1-name(13868) name-operations(0) }
	 */
	{
		ROSE_QSIG_CallingName,						NULL, 0,
			rose_enc_qsig_CallingName_ARG,			NULL,
			rose_dec_qsig_CallingName_ARG,			NULL
	},
	{
		ROSE_QSIG_CalledName,						NULL, 1,
			rose_enc_qsig_CalledName_ARG,			NULL,
			rose_dec_qsig_CalledName_ARG,			NULL
	},
	{
		ROSE_QSIG_ConnectedName,					NULL, 2,
			rose_enc_qsig_ConnectedName_ARG,		NULL,
			rose_dec_qsig_ConnectedName_ARG,		NULL
	},
	{
		ROSE_QSIG_BusyName,							NULL, 3,
			rose_enc_qsig_BusyName_ARG,				NULL,
			rose_dec_qsig_BusyName_ARG,				NULL
	},

	/*
	 * localValue's from Q.SIG SS-AOC-Operations
	 * { iso(1) standard(0) pss1-advice-of-charge(15050) advice-of-charge-operations(0) }
	 */
	{
		ROSE_QSIG_ChargeRequest,					NULL, 59,
			rose_enc_qsig_ChargeRequest_ARG,		rose_enc_qsig_ChargeRequest_RES,
			rose_dec_qsig_ChargeRequest_ARG,		rose_dec_qsig_ChargeRequest_RES
	},
	{
		ROSE_QSIG_GetFinalCharge,					NULL, 60,
			rose_enc_qsig_DummyArg_ARG,				NULL,
			rose_dec_qsig_DummyArg_ARG,				NULL
	},
	{
		ROSE_QSIG_AocFinal,							NULL, 61,
			rose_enc_qsig_AocFinal_ARG,				NULL,
			rose_dec_qsig_AocFinal_ARG,				NULL
	},
	{
		ROSE_QSIG_AocInterim,						NULL, 62,
			rose_enc_qsig_AocInterim_ARG,			NULL,
			rose_dec_qsig_AocInterim_ARG,			NULL
	},
	{
		ROSE_QSIG_AocRate,							NULL, 63,
			rose_enc_qsig_AocRate_ARG,				NULL,
			rose_dec_qsig_AocRate_ARG,				NULL
	},
	{
		ROSE_QSIG_AocComplete,						NULL, 64,
			rose_enc_qsig_AocComplete_ARG,			rose_enc_qsig_AocComplete_RES,
			rose_dec_qsig_AocComplete_ARG,			rose_dec_qsig_AocComplete_RES
	},
	{
		ROSE_QSIG_AocDivChargeReq,					NULL, 65,
			rose_enc_qsig_AocDivChargeReq_ARG,		NULL,
			rose_dec_qsig_AocDivChargeReq_ARG,		NULL
	},

	/*
	 * localValue's from Q.SIG Call-Transfer-Operations
	 * { iso(1) standard(0) pss1-call-transfer(13869) call-transfer-operations(0) }
	 */
	{
		ROSE_QSIG_CallTransferIdentify,				NULL, 7,
			rose_enc_qsig_DummyArg_ARG,				rose_enc_qsig_CallTransferIdentify_RES,
			rose_dec_qsig_DummyArg_ARG,				rose_dec_qsig_CallTransferIdentify_RES
	},
	{
		ROSE_QSIG_CallTransferAbandon,				NULL, 8,
			rose_enc_qsig_DummyArg_ARG,				NULL,
			rose_dec_qsig_DummyArg_ARG,				NULL
	},
	{
		ROSE_QSIG_CallTransferInitiate,				NULL, 9,
			rose_enc_qsig_CallTransferInitiate_ARG,	rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_CallTransferInitiate_ARG,	rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_CallTransferSetup,				NULL, 10,
			rose_enc_qsig_CallTransferSetup_ARG,	rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_CallTransferSetup_ARG,	rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_CallTransferActive,				NULL, 11,
			rose_enc_qsig_CallTransferActive_ARG,	NULL,
			rose_dec_qsig_CallTransferActive_ARG,	NULL
	},
	{
		ROSE_QSIG_CallTransferComplete,				NULL, 12,
			rose_enc_qsig_CallTransferComplete_ARG,	NULL,
			rose_dec_qsig_CallTransferComplete_ARG,	NULL
	},
	{
		ROSE_QSIG_CallTransferUpdate,				NULL, 13,
			rose_enc_qsig_CallTransferUpdate_ARG,	NULL,
			rose_dec_qsig_CallTransferUpdate_ARG,	NULL
	},
	{
		ROSE_QSIG_SubaddressTransfer,				NULL, 14,
			rose_enc_qsig_SubaddressTransfer_ARG,	NULL,
			rose_dec_qsig_SubaddressTransfer_ARG,	NULL
	},

	/*
	 * NOTE:  I do not have the specification needed to fully support this
	 * message.  Fortunately, all I have to do for this message is to switch
	 * it to the bridged call leg for 2BCT support.
	 */
	{
		ROSE_QSIG_PathReplacement,					NULL, 4,
			NULL,									NULL,
			NULL,									NULL
	},

	/*
	 * localValue's from Q.SIG Call-Diversion-Operations
	 * { iso(1) standard(0) pss1-call-diversion(13873) call-diversion-operations(0) }
	 */
	{
		ROSE_QSIG_ActivateDiversionQ,				NULL, 15,
			rose_enc_qsig_ActivateDiversionQ_ARG,	rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_ActivateDiversionQ_ARG,	rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_DeactivateDiversionQ,				NULL, 16,
			rose_enc_qsig_DeactivateDiversionQ_ARG,	rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_DeactivateDiversionQ_ARG,	rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_InterrogateDiversionQ,			NULL, 17,
			rose_enc_qsig_InterrogateDiversionQ_ARG,rose_enc_qsig_InterrogateDiversionQ_RES,
			rose_dec_qsig_InterrogateDiversionQ_ARG,rose_dec_qsig_InterrogateDiversionQ_RES
	},
	{
		ROSE_QSIG_CheckRestriction,					NULL, 18,
			rose_enc_qsig_CheckRestriction_ARG,		rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_CheckRestriction_ARG,		rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_CallRerouting,					NULL, 19,
			rose_enc_qsig_CallRerouting_ARG,		rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_CallRerouting_ARG,		rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_DivertingLegInformation1,			NULL, 20,
			rose_enc_qsig_DivertingLegInformation1_ARG,NULL,
			rose_dec_qsig_DivertingLegInformation1_ARG,NULL
	},
	{
		ROSE_QSIG_DivertingLegInformation2,			NULL, 21,
			rose_enc_qsig_DivertingLegInformation2_ARG,NULL,
			rose_dec_qsig_DivertingLegInformation2_ARG,NULL
	},
	{
		ROSE_QSIG_DivertingLegInformation3,			NULL, 22,
			rose_enc_qsig_DivertingLegInformation3_ARG,NULL,
			rose_dec_qsig_DivertingLegInformation3_ARG,NULL
	},
	{
		ROSE_QSIG_CfnrDivertedLegFailed,			NULL, 23,
			rose_enc_qsig_DummyArg_ARG,				NULL,
			rose_dec_qsig_DummyArg_ARG,				NULL
	},

	/*
	 * localValue's from Q.SIG SS-MWI-Operations
	 * { iso(1) standard(0) pss1-message-waiting-indication(15506) message-waiting-operations(0) }
	 */
	{
		ROSE_QSIG_MWIActivate,						NULL, 80,
			rose_enc_qsig_MWIActivate_ARG,			rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_MWIActivate_ARG,			rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_MWIDeactivate,					NULL, 81,
			rose_enc_qsig_MWIDeactivate_ARG,		rose_enc_qsig_DummyRes_RES,
			rose_dec_qsig_MWIDeactivate_ARG,		rose_dec_qsig_DummyRes_RES
	},
	{
		ROSE_QSIG_MWIInterrogate,					NULL, 82,
			rose_enc_qsig_MWIInterrogate_ARG,		rose_enc_qsig_MWIInterrogate_RES,
			rose_dec_qsig_MWIInterrogate_ARG,		rose_dec_qsig_MWIInterrogate_RES
	},
/* *INDENT-ON* */
};


/*! \brief Q.SIG specific error-value converion table */
static const struct rose_convert_error rose_qsig_errors[] = {
/* *INDENT-OFF* */
/*
 *		error-code,                                 oid_prefix, value
 *			encode_error_args,                      decode_error_args
 */
	/*
	 * localValue Errors from General-Error-List
	 * {ccitt identified-organization q 950 general-error-list(1)}
	 */
	{
		ROSE_ERROR_Gen_NotSubscribed,				NULL, 0,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_RejectedByNetwork,			NULL, 1,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_RejectedByUser,				NULL, 2,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_NotAvailable,				NULL, 3,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InsufficientInformation,		NULL, 5,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InvalidServedUserNr,			NULL, 6,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InvalidCallState,			NULL, 7,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_BasicServiceNotProvided,		NULL, 8,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_NotIncomingCall,				NULL, 9,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_SupplementaryServiceInteractionNotAllowed,NULL, 10,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_ResourceUnavailable,			NULL, 11,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_CallFailure,					NULL, 25,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_ProceduralError,				NULL, 43,
			NULL,									NULL
	},

	/*
	 * From various Q.SIG specifications.
	 * We will ignore the manufacturer specific extension information.
	 */
	{
		ROSE_ERROR_QSIG_Unspecified,				NULL, 1008,
			NULL,									NULL
	},

	/*
	 * localValue Errors from Q.SIG SS-AOC-Operations
	 * { iso(1) standard(0) pss1-advice-of-charge(15050) advice-of-charge-operations(0) }
	 */
	{
		ROSE_ERROR_QSIG_AOC_FreeOfCharge,			NULL, 1016,
			NULL,									NULL
	},

	/*
	 * localValue's from Q.SIG Call-Transfer-Operations
	 * { iso(1) standard(0) pss1-call-transfer(13869) call-transfer-operations(0) }
	 */
	{
		ROSE_ERROR_QSIG_CT_InvalidReroutingNumber,	NULL, 1004,
			NULL,									NULL
	},
	{
		ROSE_ERROR_QSIG_CT_UnrecognizedCallIdentity,NULL, 1005,
			NULL,									NULL
	},
	{
		ROSE_ERROR_QSIG_CT_EstablishmentFailure,	NULL, 1006,
			NULL,									NULL
	},

	/*
	 * localValue's from Q.SIG Call-Diversion-Operations
	 * { iso(1) standard(0) pss1-call-diversion(13873) call-diversion-operations(0) }
	 */
	{
		ROSE_ERROR_Div_InvalidDivertedToNr,			NULL, 12,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Div_SpecialServiceNr,			NULL, 14,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Div_DiversionToServedUserNr,		NULL, 15,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Div_NumberOfDiversionsExceeded,	NULL, 24,
			NULL,									NULL
	},
	{
		ROSE_ERROR_QSIG_Div_TemporarilyUnavailable,	NULL, 1000,
			NULL,									NULL
	},
	{
		ROSE_ERROR_QSIG_Div_NotAuthorized,			NULL, 1007,
			NULL,									NULL
	},

	/*
	 * localValue's from Q.SIG SS-MWI-Operations
	 * { iso(1) standard(0) pss1-message-waiting-indication(15506) message-waiting-operations(0) }
	 */
	{
		ROSE_ERROR_QSIG_InvalidMsgCentreId,			NULL, 1018,
			NULL,									NULL
	},
/* *INDENT-ON* */
};


/* ------------------------------------------------------------------- */


/*! \brief DMS-100 specific invoke/result encode/decode message table */
static const struct rose_convert_msg rose_dms100_msgs[] = {
/* *INDENT-OFF* */
/*
 *		operation,                                  oid_prefix, value,
 *			encode_invoke_args,                     encode_result_args,
 *			decode_invoke_args,                     decode_result_args
 */
	{
		ROSE_DMS100_RLT_OperationInd,				NULL, ROSE_DMS100_RLT_OPERATION_IND,
			NULL,									rose_enc_dms100_RLT_OperationInd_RES,
			NULL,									rose_dec_dms100_RLT_OperationInd_RES
	},
	{
		ROSE_DMS100_RLT_ThirdParty,					NULL, ROSE_DMS100_RLT_THIRD_PARTY,
			rose_enc_dms100_RLT_ThirdParty_ARG,		NULL,
			rose_dec_dms100_RLT_ThirdParty_ARG,		NULL
	},
/* *INDENT-ON* */
};


/*! \brief DMS-100 specific error-value converion table */
static const struct rose_convert_error rose_dms100_errors[] = {
/* *INDENT-OFF* */
/*
 *		error-code,                                 oid_prefix, value
 *			encode_error_args,                      decode_error_args
 */
	{
		ROSE_ERROR_DMS100_RLT_BridgeFail,			NULL, 0x10,
			NULL,									NULL
	},
	{
		ROSE_ERROR_DMS100_RLT_CallIDNotFound,		NULL, 0x11,
			NULL,									NULL
	},
	{
		ROSE_ERROR_DMS100_RLT_NotAllowed,			NULL, 0x12,
			NULL,									NULL
	},
	{
		ROSE_ERROR_DMS100_RLT_SwitchEquipCongs,		NULL, 0x13,
			NULL,									NULL
	},
/* *INDENT-ON* */
};


/* ------------------------------------------------------------------- */


/*
 * Note the first value in oid.values[] is really the first two
 * OID subidentifiers.  They are compressed using this formula:
 * First_Value = (First_Subidentifier * 40) + Second_Subidentifier
 */

static const struct asn1_oid rose_ni2_oid = {
/* *INDENT-OFF* */
	/* { iso(1) member-body(2) usa(840) ansi-t1(10005) operations(0) } */
	4, { 42, 840, 10005, 0 }
/* *INDENT-ON* */
};

/*! \brief NI2 specific invoke/result encode/decode message table */
static const struct rose_convert_msg rose_ni2_msgs[] = {
/* *INDENT-OFF* */
/*
 *		operation,                                  oid_prefix, value,
 *			encode_invoke_args,                     encode_result_args,
 *			decode_invoke_args,                     decode_result_args
 */
	/* NI2 seems to have pirated several Q.SIG messages */
	/*
	 * localValue's from Q.SIG Name-Operations
	 * { iso(1) standard(0) pss1-name(13868) name-operations(0) }
	 */
	{
		ROSE_QSIG_CallingName,						NULL, 0,
			rose_enc_qsig_CallingName_ARG,			NULL,
			rose_dec_qsig_CallingName_ARG,			NULL
	},
	{
		ROSE_QSIG_CalledName,						NULL, 1,
			rose_enc_qsig_CalledName_ARG,			NULL,
			rose_dec_qsig_CalledName_ARG,			NULL
	},
	{
		ROSE_QSIG_ConnectedName,					NULL, 2,
			rose_enc_qsig_ConnectedName_ARG,		NULL,
			rose_dec_qsig_ConnectedName_ARG,		NULL
	},
	{
		ROSE_QSIG_BusyName,							NULL, 3,
			rose_enc_qsig_BusyName_ARG,				NULL,
			rose_dec_qsig_BusyName_ARG,				NULL
	},

	{
		ROSE_NI2_InformationFollowing,				&rose_ni2_oid, 4,
			rose_enc_ni2_InformationFollowing_ARG,	NULL,
			rose_dec_ni2_InformationFollowing_ARG,	NULL
	},

	/* Also used by PRI_SWITCH_ATT4ESS and PRI_SWITCH_LUCENT5E */
	{
		ROSE_NI2_InitiateTransfer,					&rose_ni2_oid, 8,
			rose_enc_ni2_InitiateTransfer_ARG,		NULL,
			rose_dec_ni2_InitiateTransfer_ARG,		NULL
	},
/* *INDENT-ON* */
};


/*! \brief NI2 specific error-value converion table */
static const struct rose_convert_error rose_ni2_errors[] = {
/* *INDENT-OFF* */
/*
 *		error-code,                                 oid_prefix, value
 *			encode_error_args,                      decode_error_args
 */
	/*
	 * localValue Errors from General-Error-List
	 * {ccitt identified-organization q 950 general-error-list(1)}
	 */
	{
		ROSE_ERROR_Gen_NotSubscribed,				NULL, 0,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_RejectedByNetwork,			NULL, 1,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_RejectedByUser,				NULL, 2,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_NotAvailable,				NULL, 3,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InsufficientInformation,		NULL, 5,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InvalidServedUserNr,			NULL, 6,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_InvalidCallState,			NULL, 7,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_BasicServiceNotProvided,		NULL, 8,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_NotIncomingCall,				NULL, 9,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_SupplementaryServiceInteractionNotAllowed,NULL, 10,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_ResourceUnavailable,			NULL, 11,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_CallFailure,					NULL, 25,
			NULL,									NULL
	},
	{
		ROSE_ERROR_Gen_ProceduralError,				NULL, 43,
			NULL,									NULL
	},
/* *INDENT-ON* */
};


/* ------------------------------------------------------------------- */

/*!
 * \internal
 * \brief Convert the given code value to a string.
 *
 * \param code Code value to convert to a string.
 * \param arr Array to convert the code to a string.
 * \param num_elements Number of elements in the conversion array.
 *
 * \retval String version of the given code value.
 */
static const char *rose_code2str(int code, const struct rose_code_strings *arr,
	unsigned num_elements)
{
	static char invalid_code[40];

	unsigned index;

	for (index = 0; index < num_elements; ++index) {
		if (arr[index].code == code) {
			return arr[index].name;
		}
	}

	snprintf(invalid_code, sizeof(invalid_code), "Invalid code:%d 0x%X", code, code);
	return invalid_code;
}

/*!
 * \brief Convert the given operation-value to a string.
 *
 * \param operation Operation-value to convert to a string.
 *
 * \retval String version of the given operation-value.
 */
const char *rose_operation2str(enum rose_operation operation)
{
	static const struct rose_code_strings arr[] = {
/* *INDENT-OFF* */
		{ ROSE_None,                                "ROSE_None" },
		{ ROSE_Unknown,                             "ROSE_Unknown" },

		{ ROSE_ETSI_ChargingRequest,                "ROSE_ETSI_ChargingRequest" },
		{ ROSE_ETSI_AOCSCurrency,                   "ROSE_ETSI_AOCSCurrency" },
		{ ROSE_ETSI_AOCSSpecialArr,                 "ROSE_ETSI_AOCSSpecialArr" },
		{ ROSE_ETSI_AOCDCurrency,                   "ROSE_ETSI_AOCDCurrency" },
		{ ROSE_ETSI_AOCDChargingUnit,               "ROSE_ETSI_AOCDChargingUnit" },
		{ ROSE_ETSI_AOCECurrency,                   "ROSE_ETSI_AOCECurrency" },
		{ ROSE_ETSI_AOCEChargingUnit,               "ROSE_ETSI_AOCEChargingUnit" },

		{ ROSE_QSIG_CallingName,                    "ROSE_QSIG_CallingName" },
		{ ROSE_QSIG_CalledName,                     "ROSE_QSIG_CalledName" },
		{ ROSE_QSIG_ConnectedName,                  "ROSE_QSIG_ConnectedName" },
		{ ROSE_QSIG_BusyName,                       "ROSE_QSIG_BusyName" },

		{ ROSE_QSIG_ChargeRequest,                  "ROSE_QSIG_ChargeRequest" },
		{ ROSE_QSIG_GetFinalCharge,                 "ROSE_QSIG_GetFinalCharge" },
		{ ROSE_QSIG_AocFinal,                       "ROSE_QSIG_AocFinal" },
		{ ROSE_QSIG_AocInterim,                     "ROSE_QSIG_AocInterim" },
		{ ROSE_QSIG_AocRate,                        "ROSE_QSIG_AocRate" },
		{ ROSE_QSIG_AocComplete,                    "ROSE_QSIG_AocComplete" },
		{ ROSE_QSIG_AocDivChargeReq,                "ROSE_QSIG_AocDivChargeReq" },

		{ ROSE_QSIG_CallTransferIdentify,           "ROSE_QSIG_CallTransferIdentify" },
		{ ROSE_QSIG_CallTransferAbandon,            "ROSE_QSIG_CallTransferAbandon" },
		{ ROSE_QSIG_CallTransferInitiate,           "ROSE_QSIG_CallTransferInitiate" },
		{ ROSE_QSIG_CallTransferSetup,              "ROSE_QSIG_CallTransferSetup" },
		{ ROSE_QSIG_CallTransferActive,             "ROSE_QSIG_CallTransferActive" },
		{ ROSE_QSIG_CallTransferComplete,           "ROSE_QSIG_CallTransferComplete" },
		{ ROSE_QSIG_CallTransferUpdate,             "ROSE_QSIG_CallTransferUpdate" },
		{ ROSE_QSIG_SubaddressTransfer,             "ROSE_QSIG_SubaddressTransfer" },

		{ ROSE_QSIG_PathReplacement,                "ROSE_QSIG_PathReplacement" },

		{ ROSE_QSIG_ActivateDiversionQ,             "ROSE_QSIG_ActivateDiversionQ" },
		{ ROSE_QSIG_DeactivateDiversionQ,           "ROSE_QSIG_DeactivateDiversionQ" },
		{ ROSE_QSIG_InterrogateDiversionQ,          "ROSE_QSIG_InterrogateDiversionQ" },
		{ ROSE_QSIG_CheckRestriction,               "ROSE_QSIG_CheckRestriction" },
		{ ROSE_QSIG_CallRerouting,                  "ROSE_QSIG_CallRerouting" },
		{ ROSE_QSIG_DivertingLegInformation1,       "ROSE_QSIG_DivertingLegInformation1" },
		{ ROSE_QSIG_DivertingLegInformation2,       "ROSE_QSIG_DivertingLegInformation2" },
		{ ROSE_QSIG_DivertingLegInformation3,       "ROSE_QSIG_DivertingLegInformation3" },
		{ ROSE_QSIG_CfnrDivertedLegFailed,          "ROSE_QSIG_CfnrDivertedLegFailed" },

		{ ROSE_QSIG_MWIActivate,                    "ROSE_QSIG_MWIActivate" },
		{ ROSE_QSIG_MWIDeactivate,                  "ROSE_QSIG_MWIDeactivate" },
		{ ROSE_QSIG_MWIInterrogate,                 "ROSE_QSIG_MWIInterrogate" },

		{ ROSE_DMS100_RLT_OperationInd,             "ROSE_DMS100_RLT_OperationInd" },
		{ ROSE_DMS100_RLT_ThirdParty,               "ROSE_DMS100_RLT_ThirdParty" },

		{ ROSE_NI2_InformationFollowing,            "ROSE_NI2_InformationFollowing" },
		{ ROSE_NI2_InitiateTransfer,                "ROSE_NI2_InitiateTransfer" },
/* *INDENT-ON* */
	};

	return rose_code2str(operation, arr, ARRAY_LEN(arr));
}

/*!
 * \brief Convert the given error-value to a string.
 *
 * \param code Error-value to convert to a string.
 *
 * \retval String version of the given error-value.
 */
const char *rose_error2str(enum rose_error_code code)
{
	static const struct rose_code_strings arr[] = {
/* *INDENT-OFF* */
		{ ROSE_ERROR_None,                            "No error occurred" },
		{ ROSE_ERROR_Unknown,                         "Unknown error-value code" },

		{ ROSE_ERROR_Gen_NotSubscribed,               "General: Not Subscribed" },
		{ ROSE_ERROR_Gen_NotAvailable,                "General: Not Available" },
		{ ROSE_ERROR_Gen_NotImplemented,              "General: Not Implemented" },
		{ ROSE_ERROR_Gen_InvalidServedUserNr,         "General: Invalid Served User Number" },
		{ ROSE_ERROR_Gen_InvalidCallState,            "General: Invalid Call State" },
		{ ROSE_ERROR_Gen_BasicServiceNotProvided,     "General: Basic Service Not Provided" },
		{ ROSE_ERROR_Gen_NotIncomingCall,             "General: Not Incoming Call" },
		{ ROSE_ERROR_Gen_SupplementaryServiceInteractionNotAllowed,"General: Supplementary Service Interaction Not Allowed" },
		{ ROSE_ERROR_Gen_ResourceUnavailable,         "General: Resource Unavailable" },

		/* Additional Q.950 General-Errors for Q.SIG */
		{ ROSE_ERROR_Gen_RejectedByNetwork,           "General: Rejected By Network" },
		{ ROSE_ERROR_Gen_RejectedByUser,              "General: Rejected By User" },
		{ ROSE_ERROR_Gen_InsufficientInformation,     "General: Insufficient Information" },
		{ ROSE_ERROR_Gen_CallFailure,                 "General: Call Failure" },
		{ ROSE_ERROR_Gen_ProceduralError,             "General: Procedural Error" },

		{ ROSE_ERROR_Div_InvalidDivertedToNr,         "Diversion: Invalid Diverted To Number" },
		{ ROSE_ERROR_Div_SpecialServiceNr,            "Diversion: Special Service Number" },
		{ ROSE_ERROR_Div_DiversionToServedUserNr,     "Diversion: Diversion To Served User Number" },
		{ ROSE_ERROR_Div_NumberOfDiversionsExceeded,  "Diversion: Number Of Diversions Exceeded" },

		{ ROSE_ERROR_AOC_NoChargingInfoAvailable,     "AOC: No Charging Info Available" },

		/* Q.SIG specific errors */
		{ ROSE_ERROR_QSIG_Unspecified,                "Unspecified" },

		{ ROSE_ERROR_QSIG_AOC_FreeOfCharge,           "AOC: FreeOfCharge" },

		{ ROSE_ERROR_QSIG_CT_InvalidReroutingNumber,  "CT: Invalid Rerouting Number" },
		{ ROSE_ERROR_QSIG_CT_UnrecognizedCallIdentity,"CT: Unrecognized Call Identity" },
		{ ROSE_ERROR_QSIG_CT_EstablishmentFailure,    "CT: Establishment Failure" },

		{ ROSE_ERROR_QSIG_Div_TemporarilyUnavailable, "Diversion: Temporarily Unavailable" },
		{ ROSE_ERROR_QSIG_Div_NotAuthorized,          "Diversion: Not Authorized" },

		{ ROSE_ERROR_QSIG_InvalidMsgCentreId,         "MWI: Invalid Message Center ID" },

		/* DMS-100 specific errors */
		{ ROSE_ERROR_DMS100_RLT_BridgeFail,           "RLT: Bridge Fail" },
		{ ROSE_ERROR_DMS100_RLT_CallIDNotFound,       "RLT: Call ID Not Found" },
		{ ROSE_ERROR_DMS100_RLT_NotAllowed,           "RLT: Not Allowed" },
		{ ROSE_ERROR_DMS100_RLT_SwitchEquipCongs,     "RLT: Switch Equip Congs" },
/* *INDENT-ON* */
	};

	return rose_code2str(code, arr, ARRAY_LEN(arr));
}

/*!
 * \brief Convert the given reject problem-value to a string.
 *
 * \param code Reject problem-value to convert to a string.
 *
 * \retval String version of the given reject problem-value.
 */
const char *rose_reject2str(enum rose_reject_code code)
{
	static const struct rose_code_strings arr[] = {
/* *INDENT-OFF* */
		{ ROSE_REJECT_None,                           "No reject occurred" },
		{ ROSE_REJECT_Unknown,                        "Unknown reject code" },

		{ ROSE_REJECT_Gen_UnrecognizedComponent,      "General: Unrecognized Component" },
		{ ROSE_REJECT_Gen_MistypedComponent,          "General: Mistyped Component" },
		{ ROSE_REJECT_Gen_BadlyStructuredComponent,   "General: Badly Structured Component" },

		{ ROSE_REJECT_Inv_DuplicateInvocation,        "Invoke: Duplicate Invocation" },
		{ ROSE_REJECT_Inv_UnrecognizedOperation,      "Invoke: Unrecognized Operation" },
		{ ROSE_REJECT_Inv_MistypedArgument,           "Invoke: Mistyped Argument" },
		{ ROSE_REJECT_Inv_ResourceLimitation,         "Invoke: Resource Limitation" },
		{ ROSE_REJECT_Inv_InitiatorReleasing,         "Invoke: Initiator Releasing" },
		{ ROSE_REJECT_Inv_UnrecognizedLinkedID,       "Invoke: Unrecognized Linked ID" },
		{ ROSE_REJECT_Inv_LinkedResponseUnexpected,   "Invoke: Linked Response Unexpected" },
		{ ROSE_REJECT_Inv_UnexpectedChildOperation,   "Invoke: Unexpected Child Operation" },

		{ ROSE_REJECT_Res_UnrecognizedInvocation,     "Result: Unrecognized Invocation" },
		{ ROSE_REJECT_Res_ResultResponseUnexpected,   "Result: Result Response Unexpected" },
		{ ROSE_REJECT_Res_MistypedResult,             "Result: Mistyped Result" },

		{ ROSE_REJECT_Err_UnrecognizedInvocation,     "Error: Unrecognized Invocation" },
		{ ROSE_REJECT_Err_ErrorResponseUnexpected,    "Error: Error Response Unexpected" },
		{ ROSE_REJECT_Err_UnrecognizedError,          "Error: Unrecognized Error" },
		{ ROSE_REJECT_Err_UnexpectedError,            "Error: Unexpected Error" },
		{ ROSE_REJECT_Err_MistypedParameter,          "Error: Mistyped Parameter" },
/* *INDENT-ON* */
	};

	return rose_code2str(code, arr, ARRAY_LEN(arr));
}

/*!
 * \internal
 * \brief Find an operation message conversion entry using the operation code.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param operation Library operation-value code.
 *
 * \retval Message conversion entry on success.
 * \retval NULL on error.
 */
static const struct rose_convert_msg *rose_find_msg_by_op_code(struct pri *ctrl,
	enum rose_operation operation)
{
	const struct rose_convert_msg *found;
	const struct rose_convert_msg *table;
	size_t num_entries;
	size_t index;

	/* Determine which message conversion table to use */
	switch (ctrl->switchtype) {
	case PRI_SWITCH_EUROISDN_T1:
	case PRI_SWITCH_EUROISDN_E1:
		table = rose_etsi_msgs;
		num_entries = ARRAY_LEN(rose_etsi_msgs);
		break;
	case PRI_SWITCH_QSIG:
		table = rose_qsig_msgs;
		num_entries = ARRAY_LEN(rose_qsig_msgs);
		break;
	case PRI_SWITCH_DMS100:
		table = rose_dms100_msgs;
		num_entries = ARRAY_LEN(rose_dms100_msgs);
		break;
	case PRI_SWITCH_ATT4ESS:
	case PRI_SWITCH_LUCENT5E:
	case PRI_SWITCH_NI2:
		table = rose_ni2_msgs;
		num_entries = ARRAY_LEN(rose_ni2_msgs);
		break;
	default:
		return NULL;
	}

	/* Search for the table entry */
	found = NULL;
	for (index = 0; index < num_entries; ++index) {
		if (table[index].operation == operation) {
			found = &table[index];
			break;
		}
	}

	return found;
}

/*!
 * \internal
 * \brief Find an operation message conversion entry using the
 * operation-value OID value or localValue.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param oid Search for the full OID if not NULL.
 * \param local Search for the localValue if OID is NULL.
 *
 * \retval Message conversion entry on success.
 * \retval NULL on error.
 */
static const struct rose_convert_msg *rose_find_msg_by_op_val(struct pri *ctrl,
	const struct asn1_oid *oid, unsigned local)
{
	const struct rose_convert_msg *found;
	const struct rose_convert_msg *table;
	size_t num_entries;
	size_t index;
	int sub_index;

	/* Determine which message conversion table to use */
	switch (ctrl->switchtype) {
	case PRI_SWITCH_EUROISDN_T1:
	case PRI_SWITCH_EUROISDN_E1:
		table = rose_etsi_msgs;
		num_entries = ARRAY_LEN(rose_etsi_msgs);
		break;
	case PRI_SWITCH_QSIG:
		table = rose_qsig_msgs;
		num_entries = ARRAY_LEN(rose_qsig_msgs);
		break;
	case PRI_SWITCH_DMS100:
		table = rose_dms100_msgs;
		num_entries = ARRAY_LEN(rose_dms100_msgs);
		break;
	case PRI_SWITCH_ATT4ESS:
	case PRI_SWITCH_LUCENT5E:
	case PRI_SWITCH_NI2:
		table = rose_ni2_msgs;
		num_entries = ARRAY_LEN(rose_ni2_msgs);
		break;
	default:
		return NULL;
	}

	/* Search for the table entry */
	found = NULL;
	if (oid) {
		/* Search for an OID entry */
		local = oid->value[oid->num_values - 1];
		for (index = 0; index < num_entries; ++index) {
			if (table[index].value == local && table[index].oid_prefix
				&& table[index].oid_prefix->num_values == oid->num_values - 1) {
				/* Now lets match the OID prefix subidentifiers */
				for (sub_index = oid->num_values - 2; 0 <= sub_index; --sub_index) {
					if (oid->value[sub_index]
						!= table[index].oid_prefix->value[sub_index]) {
						break;
					}
				}
				if (sub_index == -1) {
					/* All of the OID subidentifiers matched */
					found = &table[index];
					break;
				}
			}
		}
	} else {
		/* Search for a localValue entry */
		for (index = 0; index < num_entries; ++index) {
			if (table[index].value == local && !table[index].oid_prefix) {
				found = &table[index];
				break;
			}
		}
	}

	return found;
}

/*!
 * \internal
 * \brief Find an error conversion entry using the error code.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param code Library error-value code.
 *
 * \retval Error conversion entry on success.
 * \retval NULL on error.
 */
static const struct rose_convert_error *rose_find_error_by_op_code(struct pri *ctrl,
	enum rose_error_code code)
{
	const struct rose_convert_error *found;
	const struct rose_convert_error *table;
	size_t num_entries;
	size_t index;

	/* Determine which error conversion table to use */
	switch (ctrl->switchtype) {
	case PRI_SWITCH_EUROISDN_T1:
	case PRI_SWITCH_EUROISDN_E1:
		table = rose_etsi_errors;
		num_entries = ARRAY_LEN(rose_etsi_errors);
		break;
	case PRI_SWITCH_QSIG:
		table = rose_qsig_errors;
		num_entries = ARRAY_LEN(rose_qsig_errors);
		break;
	case PRI_SWITCH_DMS100:
		table = rose_dms100_errors;
		num_entries = ARRAY_LEN(rose_dms100_errors);
		break;
	case PRI_SWITCH_ATT4ESS:
	case PRI_SWITCH_LUCENT5E:
	case PRI_SWITCH_NI2:
		table = rose_ni2_errors;
		num_entries = ARRAY_LEN(rose_ni2_errors);
		break;
	default:
		return NULL;
	}

	/* Search for the table entry */
	found = NULL;
	for (index = 0; index < num_entries; ++index) {
		if (table[index].code == code) {
			found = &table[index];
			break;
		}
	}

	return found;
}

/*!
 * \internal
 * \brief Find an error conversion entry using the
 * error-value OID value or localValue.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param oid Search for the full OID if not NULL.
 * \param local Search for the localValue if OID is NULL.
 *
 * \retval Error conversion entry on success.
 * \retval NULL on error.
 */
static const struct rose_convert_error *rose_find_error_by_op_val(struct pri *ctrl,
	const struct asn1_oid *oid, unsigned local)
{
	const struct rose_convert_error *found;
	const struct rose_convert_error *table;
	size_t num_entries;
	size_t index;
	int sub_index;

	/* Determine which error conversion table to use */
	switch (ctrl->switchtype) {
	case PRI_SWITCH_EUROISDN_T1:
	case PRI_SWITCH_EUROISDN_E1:
		table = rose_etsi_errors;
		num_entries = ARRAY_LEN(rose_etsi_errors);
		break;
	case PRI_SWITCH_QSIG:
		table = rose_qsig_errors;
		num_entries = ARRAY_LEN(rose_qsig_errors);
		break;
	case PRI_SWITCH_DMS100:
		table = rose_dms100_errors;
		num_entries = ARRAY_LEN(rose_dms100_errors);
		break;
	case PRI_SWITCH_ATT4ESS:
	case PRI_SWITCH_LUCENT5E:
	case PRI_SWITCH_NI2:
		table = rose_ni2_errors;
		num_entries = ARRAY_LEN(rose_ni2_errors);
		break;
	default:
		return NULL;
	}

	/* Search for the table entry */
	found = NULL;
	if (oid) {
		/* Search for an OID entry */
		local = oid->value[oid->num_values - 1];
		for (index = 0; index < num_entries; ++index) {
			if (table[index].value == local && table[index].oid_prefix
				&& table[index].oid_prefix->num_values == oid->num_values - 1) {
				/* Now lets match the OID prefix subidentifiers */
				for (sub_index = oid->num_values - 2; 0 <= sub_index; --sub_index) {
					if (oid->value[sub_index]
						!= table[index].oid_prefix->value[sub_index]) {
						break;
					}
				}
				if (sub_index == -1) {
					/* All of the OID subidentifiers matched */
					found = &table[index];
					break;
				}
			}
		}
	} else {
		/* Search for a localValue entry */
		for (index = 0; index < num_entries; ++index) {
			if (table[index].value == local && !table[index].oid_prefix) {
				found = &table[index];
				break;
			}
		}
	}

	return found;
}

/*!
 * \internal
 * \brief Encode the Facility ie component operation-value.
 *
 * \param pos Starting position to encode the operation-value.
 * \param end End of ASN.1 encoding data buffer.
 * \param oid_prefix Encode as an OID if not NULL.
 * \param local Encode as a localValue if oid_prefix is NULL
 * else it is the last OID subidentifier.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
static unsigned char *rose_enc_operation_value(unsigned char *pos, unsigned char *end,
	const struct asn1_oid *oid_prefix, unsigned local)
{
	struct asn1_oid oid;

	if (oid_prefix) {
		if (ARRAY_LEN(oid_prefix->value) <= oid_prefix->num_values) {
			return NULL;
		}
		oid = *oid_prefix;
		oid.value[oid.num_values++] = local;
		return asn1_enc_oid(pos, end, ASN1_TYPE_OBJECT_IDENTIFIER, &oid);
	} else {
		return asn1_enc_int(pos, end, ASN1_TYPE_INTEGER, local);
	}
}

/*! \brief Mapped to rose_enc_operation_value() */
#define rose_enc_error_value(pos, end, oid_prefix, local)	\
	rose_enc_operation_value(pos, end, oid_prefix, local)

/*!
 * \brief Encode the invoke component for a ROSE message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode ASN.1 message.
 * \param end End of ASN.1 encoding data buffer.
 * \param msg ROSE invoke message to encode.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
unsigned char *rose_encode_invoke(struct pri *ctrl, unsigned char *pos,
	unsigned char *end, const struct rose_msg_invoke *msg)
{
	const struct rose_convert_msg *convert;
	unsigned char *seq_len;

	convert = rose_find_msg_by_op_code(ctrl, msg->operation);
	if (!convert) {
		return NULL;
	}

	ASN1_CONSTRUCTED_BEGIN(seq_len, pos, end, ROSE_TAG_COMPONENT_INVOKE);

	ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_TYPE_INTEGER, msg->invoke_id));
	if (msg->linked_id_present) {
		ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 0,
			msg->linked_id));
	}
	ASN1_CALL(pos, rose_enc_operation_value(pos, end, convert->oid_prefix,
		convert->value));

	if (convert->encode_invoke_args) {
		ASN1_CALL(pos, convert->encode_invoke_args(ctrl, pos, end, &msg->args));
	}

	ASN1_CONSTRUCTED_END(seq_len, pos, end);

	return pos;
}

/*!
 * \brief Encode the result component for a ROSE message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode ASN.1 message.
 * \param end End of ASN.1 encoding data buffer.
 * \param msg ROSE result message to encode.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
unsigned char *rose_encode_result(struct pri *ctrl, unsigned char *pos,
	unsigned char *end, const struct rose_msg_result *msg)
{
	const struct rose_convert_msg *convert;
	unsigned char *seq_len;
	unsigned char *op_seq_len;

	ASN1_CONSTRUCTED_BEGIN(seq_len, pos, end, ROSE_TAG_COMPONENT_RESULT);

	ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_TYPE_INTEGER, msg->invoke_id));

	if (msg->operation != ROSE_None) {
		convert = rose_find_msg_by_op_code(ctrl, msg->operation);
		if (!convert) {
			return NULL;
		}

		ASN1_CONSTRUCTED_BEGIN(op_seq_len, pos, end, ASN1_TYPE_SEQUENCE);

		ASN1_CALL(pos, rose_enc_operation_value(pos, end, convert->oid_prefix,
			convert->value));

		if (convert->encode_result_args) {
			ASN1_CALL(pos, convert->encode_result_args(ctrl, pos, end, &msg->args));
		}

		ASN1_CONSTRUCTED_END(op_seq_len, pos, end);
	}

	ASN1_CONSTRUCTED_END(seq_len, pos, end);

	return pos;
}

/*!
 * \brief Encode the error component for a ROSE message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode ASN.1 message.
 * \param end End of ASN.1 encoding data buffer.
 * \param msg ROSE error message to encode.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
unsigned char *rose_encode_error(struct pri *ctrl, unsigned char *pos,
	unsigned char *end, const struct rose_msg_error *msg)
{
	const struct rose_convert_error *convert;
	unsigned char *seq_len;

	convert = rose_find_error_by_op_code(ctrl, msg->code);
	if (!convert) {
		return NULL;
	}

	ASN1_CONSTRUCTED_BEGIN(seq_len, pos, end, ROSE_TAG_COMPONENT_ERROR);

	ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_TYPE_INTEGER, msg->invoke_id));
	ASN1_CALL(pos, rose_enc_error_value(pos, end, convert->oid_prefix, convert->value));
	if (convert->encode_error_args) {
		ASN1_CALL(pos, convert->encode_error_args(ctrl, pos, end, &msg->args));
	}

	ASN1_CONSTRUCTED_END(seq_len, pos, end);

	return pos;
}

/*!
 * \brief Encode the reject component for a ROSE message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode ASN.1 message.
 * \param end End of ASN.1 encoding data buffer.
 * \param msg ROSE reject message to encode.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
unsigned char *rose_encode_reject(struct pri *ctrl, unsigned char *pos,
	unsigned char *end, const struct rose_msg_reject *msg)
{
	unsigned char *seq_len;
	unsigned tag;

	ASN1_CONSTRUCTED_BEGIN(seq_len, pos, end, ROSE_TAG_COMPONENT_REJECT);

	/* Encode Invoke ID */
	if (msg->invoke_id_present) {
		ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_TYPE_INTEGER, msg->invoke_id));
	} else {
		ASN1_CALL(pos, asn1_enc_null(pos, end, ASN1_TYPE_NULL));
	}

	/* Encode the reject problem */
	switch (msg->code & ~0xFF) {
	case ROSE_REJECT_BASE(ROSE_REJECT_BASE_General):
		tag = ASN1_CLASS_CONTEXT_SPECIFIC | 0;
		break;
	case ROSE_REJECT_BASE(ROSE_REJECT_BASE_Invoke):
		tag = ASN1_CLASS_CONTEXT_SPECIFIC | 1;
		break;
	case ROSE_REJECT_BASE(ROSE_REJECT_BASE_Result):
		tag = ASN1_CLASS_CONTEXT_SPECIFIC | 2;
		break;
	case ROSE_REJECT_BASE(ROSE_REJECT_BASE_Error):
		tag = ASN1_CLASS_CONTEXT_SPECIFIC | 3;
		break;
	default:
		return NULL;
	}
	ASN1_CALL(pos, asn1_enc_int(pos, end, tag, msg->code & 0xFF));

	ASN1_CONSTRUCTED_END(seq_len, pos, end);

	return pos;
}

/*!
 * \brief Encode the ROSE message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode ASN.1 message.
 * \param end End of ASN.1 encoding data buffer.
 * \param msg ROSE message to encode.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 *
 * \note This function only encodes the ROSE contents.  It does not include
 * the protocol profile, NFE, NPP, and interpretation octets defined in
 * a facility ie that may precede the ROSE contents.  These header octets
 * may already be stored in the encompassing buffer before the starting
 * position given here.
 */
unsigned char *rose_encode(struct pri *ctrl, unsigned char *pos, unsigned char *end,
	const struct rose_message *msg)
{
	switch (msg->type) {
	case ROSE_COMP_TYPE_INVOKE:
		pos = rose_encode_invoke(ctrl, pos, end, &msg->component.invoke);
		break;
	case ROSE_COMP_TYPE_RESULT:
		pos = rose_encode_result(ctrl, pos, end, &msg->component.result);
		break;
	case ROSE_COMP_TYPE_ERROR:
		pos = rose_encode_error(ctrl, pos, end, &msg->component.error);
		break;
	case ROSE_COMP_TYPE_REJECT:
		pos = rose_encode_reject(ctrl, pos, end, &msg->component.reject);
		break;
	default:
		pos = NULL;
		break;
	}

	return pos;
}

/*!
 * \internal
 * \brief Encode the NetworkFacilityExtension type.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode ASN.1 component.
 * \param end End of ASN.1 encoding data buffer.
 * \param nfe Network Facility Extension information to encode.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
static unsigned char *fac_enc_nfe(struct pri *ctrl, unsigned char *pos,
	unsigned char *end, const struct facNetworkFacilityExtension *nfe)
{
	unsigned char *seq_len;
	unsigned char *explicit_len;

	ASN1_CONSTRUCTED_BEGIN(seq_len, pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 10);

	ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 0,
		nfe->source_entity));
	if (nfe->source_number.length) {
		/* EXPLICIT tag */
		ASN1_CONSTRUCTED_BEGIN(explicit_len, pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 1);
		ASN1_CALL(pos, rose_enc_PartyNumber(ctrl, pos, end, &nfe->source_number));
		ASN1_CONSTRUCTED_END(explicit_len, pos, end);
	}
	ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 2,
		nfe->destination_entity));
	if (nfe->destination_number.length) {
		/* EXPLICIT tag */
		ASN1_CONSTRUCTED_BEGIN(explicit_len, pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 3);
		ASN1_CALL(pos, rose_enc_PartyNumber(ctrl, pos, end, &nfe->destination_number));
		ASN1_CONSTRUCTED_END(explicit_len, pos, end);
	}

	ASN1_CONSTRUCTED_END(seq_len, pos, end);

	return pos;
}

/*!
 * \brief Encode the facility extension header.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode ASN.1 component.
 * \param end End of ASN.1 encoding data buffer.
 * \param header Facility extension information to encode.
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
unsigned char *fac_enc_extension_header(struct pri *ctrl, unsigned char *pos,
	unsigned char *end, const struct fac_extension_header *header)
{
	if (header->nfe_present) {
		ASN1_CALL(pos, fac_enc_nfe(ctrl, pos, end, &header->nfe));
	}
	if (header->npp_present) {
		ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 18,
			header->npp));
	}
	if (header->interpretation_present) {
		ASN1_CALL(pos, asn1_enc_int(pos, end, ASN1_CLASS_CONTEXT_SPECIFIC | 11,
			header->interpretation));
	}

	return pos;
}

/*!
 * \brief Encode the facility ie contents header.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position to encode the facility ie contents.
 * \param end End of facility ie contents encoding data buffer.
 * \param header Facility extension header data to encode (NULL if none).
 *
 * \retval Start of the next ASN.1 component to encode on success.
 * \retval NULL on error.
 */
unsigned char *facility_encode_header(struct pri *ctrl, unsigned char *pos,
	unsigned char *end, const struct fac_extension_header *header)
{
	/* Make sure we have some room. */
	if (end < pos + 2) {
		return NULL;
	}

	switch (ctrl->switchtype) {
	case PRI_SWITCH_EUROISDN_T1:
	case PRI_SWITCH_EUROISDN_E1:
		*pos++ = 0x80 | Q932_PROTOCOL_ROSE;
		header = NULL;
		break;
	case PRI_SWITCH_QSIG:
		*pos++ = 0x80 | Q932_PROTOCOL_EXTENSIONS;
		break;
	case PRI_SWITCH_DMS100:
		*pos++ = Q932_PROTOCOL_ROSE;	/* DON'T set the EXT bit yet. */
		*pos++ = 0x80 | ROSE_DMS100_RLT_SERVICE_ID;
		header = NULL;
		break;
	case PRI_SWITCH_ATT4ESS:
	case PRI_SWITCH_LUCENT5E:
	case PRI_SWITCH_NI2:
		if (header) {
			*pos++ = 0x80 | Q932_PROTOCOL_EXTENSIONS;
		} else {
			*pos++ = 0x80 | Q932_PROTOCOL_ROSE;
		}
		break;
	default:
		return NULL;
	}

	if (header) {
		ASN1_CALL(pos, fac_enc_extension_header(ctrl, pos, end, header));
	}

	return pos;
}

/*!
 * \internal
 * \brief Decode the ROSE invoke message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param tag Component tag that identified this structure.
 * \param pos Starting position of the ASN.1 component length.
 * \param end End of ASN.1 decoding data buffer.
 * \param msg ROSE invoke message data to fill.
 *
 * \retval Start of the next ASN.1 component on success.
 * \retval NULL on error.
 */
static const unsigned char *rose_decode_invoke(struct pri *ctrl, unsigned tag,
	const unsigned char *pos, const unsigned char *end, struct rose_msg_invoke *msg)
{
	int32_t value;
	int length;
	int seq_offset;
	const unsigned char *seq_end;
	const struct rose_convert_msg *convert;
	struct asn1_oid oid;
	unsigned local;

	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "INVOKE Component %s\n", asn1_tag2str(tag));
	}
	ASN1_CALL(pos, asn1_dec_length(pos, end, &length));
	ASN1_END_SETUP(seq_end, seq_offset, length, pos, end);

	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	ASN1_CHECK_TAG(ctrl, tag, tag, ASN1_TYPE_INTEGER);
	ASN1_CALL(pos, asn1_dec_int(ctrl, "invokeId", tag, pos, seq_end, &value));
	msg->invoke_id = value;

	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	if (tag == (ASN1_CLASS_CONTEXT_SPECIFIC | 0)) {
		ASN1_CALL(pos, asn1_dec_int(ctrl, "linkedId", tag, pos, seq_end, &value));
		msg->linked_id = value;
		msg->linked_id_present = 1;

		ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	} else {
		msg->linked_id_present = 0;
	}

	/* Decode operation-value */
	switch (tag) {
	case ASN1_TYPE_INTEGER:
		ASN1_CALL(pos, asn1_dec_int(ctrl, "operationValue", tag, pos, seq_end, &value));
		local = value;
		oid.num_values = 0;
		break;
	case ASN1_TYPE_OBJECT_IDENTIFIER:
		ASN1_CALL(pos, asn1_dec_oid(ctrl, "operationValue", tag, pos, seq_end, &oid));
		local = 0;
		break;
	default:
		ASN1_DID_NOT_EXPECT_TAG(ctrl, tag);
		return NULL;
	}
	convert = rose_find_msg_by_op_val(ctrl, (oid.num_values == 0) ? NULL : &oid, local);
	if (convert) {
		msg->operation = convert->operation;
	} else {
		msg->operation = ROSE_Unknown;
	}
	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "  operationValue = %s\n", rose_operation2str(msg->operation));
	}

	/* Decode any expected invoke arguments */
	if (convert && convert->decode_invoke_args) {
		ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
		ASN1_CALL(pos, convert->decode_invoke_args(ctrl, tag, pos, seq_end, &msg->args));
	}

	ASN1_END_FIXUP(ctrl, pos, seq_offset, seq_end, end);

	return pos;
}

/*!
 * \internal
 * \brief Decode the ROSE result message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param tag Component tag that identified this structure.
 * \param pos Starting position of the ASN.1 component length.
 * \param end End of ASN.1 decoding data buffer.
 * \param msg ROSE result message data to fill.
 *
 * \retval Start of the next ASN.1 component on success.
 * \retval NULL on error.
 */
static const unsigned char *rose_decode_result(struct pri *ctrl, unsigned tag,
	const unsigned char *pos, const unsigned char *end, struct rose_msg_result *msg)
{
	int32_t value;
	int length;
	int seq_offset;
	int op_seq_offset;
	const unsigned char *seq_end;
	const unsigned char *op_seq_end;
	const struct rose_convert_msg *convert;
	struct asn1_oid oid;
	unsigned local;

	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "RESULT Component %s\n", asn1_tag2str(tag));
	}
	ASN1_CALL(pos, asn1_dec_length(pos, end, &length));
	ASN1_END_SETUP(seq_end, seq_offset, length, pos, end);

	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	ASN1_CHECK_TAG(ctrl, tag, tag, ASN1_TYPE_INTEGER);
	ASN1_CALL(pos, asn1_dec_int(ctrl, "invokeId", tag, pos, seq_end, &value));
	msg->invoke_id = value;

	/* Decode optional operation sequence */
	if (pos < seq_end && *pos != ASN1_INDEF_TERM) {
		ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
		ASN1_CHECK_TAG(ctrl, tag, tag, ASN1_TAG_SEQUENCE);
		if (ctrl->debug & PRI_DEBUG_APDU) {
			pri_message(ctrl, "  operation %s\n", asn1_tag2str(tag));
		}
		ASN1_CALL(pos, asn1_dec_length(pos, seq_end, &length));
		ASN1_END_SETUP(op_seq_end, op_seq_offset, length, pos, seq_end);

		/* Decode operation-value */
		ASN1_CALL(pos, asn1_dec_tag(pos, op_seq_end, &tag));
		switch (tag) {
		case ASN1_TYPE_INTEGER:
			ASN1_CALL(pos, asn1_dec_int(ctrl, "operationValue", tag, pos, op_seq_end,
				&value));
			local = value;
			oid.num_values = 0;
			break;
		case ASN1_TYPE_OBJECT_IDENTIFIER:
			ASN1_CALL(pos, asn1_dec_oid(ctrl, "operationValue", tag, pos, op_seq_end,
				&oid));
			local = 0;
			break;
		default:
			ASN1_DID_NOT_EXPECT_TAG(ctrl, tag);
			return NULL;
		}
		convert =
			rose_find_msg_by_op_val(ctrl, (oid.num_values == 0) ? NULL : &oid, local);
		if (convert) {
			msg->operation = convert->operation;
		} else {
			msg->operation = ROSE_Unknown;
		}
		if (ctrl->debug & PRI_DEBUG_APDU) {
			pri_message(ctrl, "  operationValue = %s\n",
				rose_operation2str(msg->operation));
		}

		/* Decode any expected result arguments */
		if (convert && convert->decode_result_args) {
			ASN1_CALL(pos, asn1_dec_tag(pos, op_seq_end, &tag));
			ASN1_CALL(pos, convert->decode_result_args(ctrl, tag, pos, op_seq_end,
				&msg->args));
		}

		ASN1_END_FIXUP(ctrl, pos, op_seq_offset, op_seq_end, seq_end);
	} else {
		msg->operation = ROSE_None;
	}

	ASN1_END_FIXUP(ctrl, pos, seq_offset, seq_end, end);

	return pos;
}

/*!
 * \internal
 * \brief Decode the ROSE error message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param tag Component tag that identified this structure.
 * \param pos Starting position of the ASN.1 component length.
 * \param end End of ASN.1 decoding data buffer.
 * \param msg ROSE error message data to fill.
 *
 * \retval Start of the next ASN.1 component on success.
 * \retval NULL on error.
 */
static const unsigned char *rose_decode_error(struct pri *ctrl, unsigned tag,
	const unsigned char *pos, const unsigned char *end, struct rose_msg_error *msg)
{
	int32_t value;
	int length;
	int seq_offset;
	const unsigned char *seq_end;
	const struct rose_convert_error *convert;
	struct asn1_oid oid;
	unsigned local;

	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "ERROR Component %s\n", asn1_tag2str(tag));
	}
	ASN1_CALL(pos, asn1_dec_length(pos, end, &length));
	ASN1_END_SETUP(seq_end, seq_offset, length, pos, end);

	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	ASN1_CHECK_TAG(ctrl, tag, tag, ASN1_TYPE_INTEGER);
	ASN1_CALL(pos, asn1_dec_int(ctrl, "invokeId", tag, pos, seq_end, &value));
	msg->invoke_id = value;

	/* Decode error-value */
	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	switch (tag) {
	case ASN1_TYPE_INTEGER:
		ASN1_CALL(pos, asn1_dec_int(ctrl, "errorValue", tag, pos, seq_end, &value));
		local = value;
		oid.num_values = 0;
		break;
	case ASN1_TYPE_OBJECT_IDENTIFIER:
		ASN1_CALL(pos, asn1_dec_oid(ctrl, "errorValue", tag, pos, seq_end, &oid));
		local = 0;
		break;
	default:
		ASN1_DID_NOT_EXPECT_TAG(ctrl, tag);
		return NULL;
	}
	convert =
		rose_find_error_by_op_val(ctrl, (oid.num_values == 0) ? NULL : &oid, local);
	if (convert) {
		msg->code = convert->code;
	} else {
		msg->code = ROSE_ERROR_Unknown;
	}
	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "  errorValue = %s\n", rose_error2str(msg->code));
	}

	/* Decode any expected error parameters */
	if (convert && convert->decode_error_args) {
		ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
		ASN1_CALL(pos, convert->decode_error_args(ctrl, tag, pos, seq_end, &msg->args));
	}

	ASN1_END_FIXUP(ctrl, pos, seq_offset, seq_end, end);

	return pos;
}

/*!
 * \internal
 * \brief Decode the ROSE reject message.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param tag Component tag that identified this structure.
 * \param pos Starting position of the ASN.1 component length.
 * \param end End of ASN.1 decoding data buffer.
 * \param msg ROSE reject message data to fill.
 *
 * \retval Start of the next ASN.1 component on success.
 * \retval NULL on error.
 */
static const unsigned char *rose_decode_reject(struct pri *ctrl, unsigned tag,
	const unsigned char *pos, const unsigned char *end, struct rose_msg_reject *msg)
{
	int32_t value;
	int length;
	int seq_offset;
	const unsigned char *seq_end;

	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "REJECT Component %s\n", asn1_tag2str(tag));
	}
	ASN1_CALL(pos, asn1_dec_length(pos, end, &length));
	ASN1_END_SETUP(seq_end, seq_offset, length, pos, end);

	/* Invoke ID choice */
	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	switch (tag) {
	case ASN1_TYPE_INTEGER:
		ASN1_CALL(pos, asn1_dec_int(ctrl, "invokeId", tag, pos, seq_end, &value));
		msg->invoke_id = value;
		msg->invoke_id_present = 1;
		break;
	case ASN1_TYPE_NULL:
		ASN1_CALL(pos, asn1_dec_null(ctrl, "invokeId", tag, pos, seq_end));
		msg->invoke_id_present = 0;
		break;
	default:
		ASN1_DID_NOT_EXPECT_TAG(ctrl, tag);
		return NULL;
	}

	/* Problem choice */
	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	switch (tag) {
	case ASN1_CLASS_CONTEXT_SPECIFIC | 0:
		ASN1_CALL(pos, asn1_dec_int(ctrl, "problemGeneral", tag, pos, seq_end, &value));
		msg->code = ROSE_REJECT_BASE(ROSE_REJECT_BASE_General) | (value & 0xFF);
		break;
	case ASN1_CLASS_CONTEXT_SPECIFIC | 1:
		ASN1_CALL(pos, asn1_dec_int(ctrl, "problemInvoke", tag, pos, seq_end, &value));
		msg->code = ROSE_REJECT_BASE(ROSE_REJECT_BASE_Invoke) | (value & 0xFF);
		break;
	case ASN1_CLASS_CONTEXT_SPECIFIC | 2:
		ASN1_CALL(pos, asn1_dec_int(ctrl, "problemResult", tag, pos, seq_end, &value));
		msg->code = ROSE_REJECT_BASE(ROSE_REJECT_BASE_Result) | (value & 0xFF);
		break;
	case ASN1_CLASS_CONTEXT_SPECIFIC | 3:
		ASN1_CALL(pos, asn1_dec_int(ctrl, "problemError", tag, pos, seq_end, &value));
		msg->code = ROSE_REJECT_BASE(ROSE_REJECT_BASE_Error) | (value & 0xFF);
		break;
	default:
		ASN1_DID_NOT_EXPECT_TAG(ctrl, tag);
		return NULL;
	}
	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "  problem = %s\n", rose_reject2str(msg->code));
	}

	ASN1_END_FIXUP(ctrl, pos, seq_offset, seq_end, end);

	return pos;
}

/*!
 * \brief Decode the ROSE message into the given buffer.
 *
 * \param ctrl D channel controller for diagnostic messages or global options.
 * \param pos Starting position of the ASN.1 component.
 * \param end End of ASN.1 decoding data buffer.
 * \param msg Decoded ROSE message contents.
 *
 * \retval Start of the next ASN.1 component on success.
 * \retval NULL on error.
 *
 * \note This function only decodes the ROSE contents.  It does not check
 * for the protocol profile, NFE, NPP, and interpretation octets defined in
 * a facility ie that may preceed the ROSE contents.  These header octets
 * may already have been consumed from the encompasing buffer before the
 * buffer given here.
 */
const unsigned char *rose_decode(struct pri *ctrl, const unsigned char *pos,
	const unsigned char *end, struct rose_message *msg)
{
	unsigned tag;

	ASN1_CALL(pos, asn1_dec_tag(pos, end, &tag));
	switch (tag) {
	case ROSE_TAG_COMPONENT_INVOKE:
		msg->type = ROSE_COMP_TYPE_INVOKE;
		ASN1_CALL(pos, rose_decode_invoke(ctrl, tag, pos, end, &msg->component.invoke));
		break;
	case ROSE_TAG_COMPONENT_RESULT:
		msg->type = ROSE_COMP_TYPE_RESULT;
		ASN1_CALL(pos, rose_decode_result(ctrl, tag, pos, end, &msg->component.result));
		break;
	case ROSE_TAG_COMPONENT_ERROR:
		msg->type = ROSE_COMP_TYPE_ERROR;
		ASN1_CALL(pos, rose_decode_error(ctrl, tag, pos, end, &msg->component.error));
		break;
	case ROSE_TAG_COMPONENT_REJECT:
		msg->type = ROSE_COMP_TYPE_REJECT;
		ASN1_CALL(pos, rose_decode_reject(ctrl, tag, pos, end, &msg->component.reject));
		break;
	default:
		msg->type = ROSE_COMP_TYPE_INVALID;
		ASN1_DID_NOT_EXPECT_TAG(ctrl, tag);
		return NULL;
	}

	if (pos < end) {
		if (ctrl->debug & PRI_DEBUG_APDU) {
			pri_message(ctrl, "  %u byte(s) of trailing data not consumed.\n",
				(unsigned) (end - pos));
		}
	}

	return pos;
}

/*!
 * \internal
 * \brief Decode the NetworkFacilityExtension argument parameters.
 *
 * \param ctrl D channel controller for any diagnostic messages.
 * \param name Field name
 * \param tag Component tag that identified this production.
 * \param pos Starting position of the ASN.1 component length.
 * \param end End of ASN.1 decoding data buffer.
 * \param nfe Parameter storage to fill.
 *
 * \retval Start of the next ASN.1 component on success.
 * \retval NULL on error.
 */
static const unsigned char *fac_dec_nfe(struct pri *ctrl, const char *name, unsigned tag,
	const unsigned char *pos, const unsigned char *end,
	struct facNetworkFacilityExtension *nfe)
{
	int length;
	int seq_offset;
	int explicit_offset;
	const unsigned char *seq_end;
	const unsigned char *explicit_end;
	const unsigned char *save_pos;
	int32_t value;

	if (ctrl->debug & PRI_DEBUG_APDU) {
		pri_message(ctrl, "  %s NetworkFacilityExtension %s\n", name, asn1_tag2str(tag));
	}
	ASN1_CALL(pos, asn1_dec_length(pos, end, &length));
	ASN1_END_SETUP(seq_end, seq_offset, length, pos, end);

	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	ASN1_CHECK_TAG(ctrl, tag, tag, ASN1_CLASS_CONTEXT_SPECIFIC | 0);
	ASN1_CALL(pos, asn1_dec_int(ctrl, "sourceEntity", tag, pos, seq_end, &value));
	nfe->source_entity = value;

	ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	if (tag == (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_PC_CONSTRUCTED | 1)) {
		/* Remove EXPLICIT tag */
		if (ctrl->debug & PRI_DEBUG_APDU) {
			pri_message(ctrl, "  Explicit %s\n", asn1_tag2str(tag));
		}
		ASN1_CALL(pos, asn1_dec_length(pos, seq_end, &length));
		ASN1_END_SETUP(explicit_end, explicit_offset, length, pos, seq_end);

		ASN1_CALL(pos, asn1_dec_tag(pos, explicit_end, &tag));
		ASN1_CALL(pos, rose_dec_PartyNumber(ctrl, "sourceEntityAddress", tag, pos,
			seq_end, &nfe->source_number));

		ASN1_END_FIXUP(ctrl, pos, explicit_offset, explicit_end, seq_end);

		ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
	} else {
		nfe->source_number.length = 0;
	}

	ASN1_CHECK_TAG(ctrl, tag, tag, ASN1_CLASS_CONTEXT_SPECIFIC | 2);
	ASN1_CALL(pos, asn1_dec_int(ctrl, "destinationEntity", tag, pos, seq_end, &value));
	nfe->destination_entity = value;

	nfe->destination_number.length = 0;
	if (pos < seq_end && *pos != ASN1_INDEF_TERM) {
		save_pos = pos;
		ASN1_CALL(pos, asn1_dec_tag(pos, seq_end, &tag));
		if (tag == (ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_PC_CONSTRUCTED | 3)) {
			/* Remove EXPLICIT tag */
			if (ctrl->debug & PRI_DEBUG_APDU) {
				pri_message(ctrl, "  Explicit %s\n", asn1_tag2str(tag));
			}
			ASN1_CALL(pos, asn1_dec_length(pos, seq_end, &length));
			ASN1_END_SETUP(explicit_end, explicit_offset, length, pos, seq_end);

			ASN1_CALL(pos, asn1_dec_tag(pos, explicit_end, &tag));
			ASN1_CALL(pos, rose_dec_PartyNumber(ctrl, "destinationEntityAddress", tag,
				pos, seq_end, &nfe->destination_number));

			ASN1_END_FIXUP(ctrl, pos, explicit_offset, explicit_end, seq_end);
		} else {
			pos = save_pos;
		}
	}

	ASN1_END_FIXUP(ctrl, pos, seq_offset, seq_end, end);

	return pos;
}

/*!
 * \brief Decode the extension header argument parameters.
 *
 * \param ctrl D channel controller for any diagnostic messages.
 * \param pos Starting position of the ASN.1 component.
 * \param end End of ASN.1 decoding data buffer.
 * \param header Parameter storage to fill.
 *
 * \retval Start of the next ASN.1 component on success.
 * \retval NULL on error.
 */
const unsigned char *fac_dec_extension_header(struct pri *ctrl, const unsigned char *pos,
	const unsigned char *end, struct fac_extension_header *header)
{
	int32_t value;
	unsigned tag;
	const unsigned char *save_pos;

	/*
	 * For simplicity we are not checking the order of
	 * the optional header components.
	 */
	header->nfe_present = 0;
	header->npp_present = 0;
	header->interpretation_present = 0;
	while (pos < end) {
		save_pos = pos;
		ASN1_CALL(pos, asn1_dec_tag(pos, end, &tag));
		switch (tag) {
		case ASN1_CLASS_CONTEXT_SPECIFIC | ASN1_PC_CONSTRUCTED | 10:
			ASN1_CALL(pos, fac_dec_nfe(ctrl, "nfe", tag, pos, end, &header->nfe));
			header->nfe_present = 1;
			break;
		case ASN1_CLASS_CONTEXT_SPECIFIC | 18:
			ASN1_CALL(pos, asn1_dec_int(ctrl, "networkProtocolProfile", tag, pos, end,
				&value));
			header->npp = value;
			header->npp_present = 1;
			break;
		case ASN1_CLASS_CONTEXT_SPECIFIC | 11:
			ASN1_CALL(pos, asn1_dec_int(ctrl, "interpretation", tag, pos, end, &value));
			header->interpretation = value;
			header->interpretation_present = 1;
			break;
		default:
			pos = save_pos;
			goto cancel_options;
		}
	}
cancel_options:;

	return pos;
}

/*!
 * \brief Decode the facility ie contents header.
 *
 * \param ctrl D channel controller for any diagnostic messages.
 * \param pos Starting position of the facility ie contents.
 * \param end End of facility ie contents.
 * \param header Parameter storage to fill.
 *
 * \retval Start of the next ASN.1 component on success (ROSE message).
 * \retval NULL on error.
 */
const unsigned char *facility_decode_header(struct pri *ctrl, const unsigned char *pos,
	const unsigned char *end, struct fac_extension_header *header)
{
	/* Make sure we have enough room for the protocol profile ie octet(s) */
	if (end < pos + 2) {
		return NULL;
	}
	switch (*pos & Q932_PROTOCOL_MASK) {
	case Q932_PROTOCOL_ROSE:
	case Q932_PROTOCOL_EXTENSIONS:
		break;
	default:
		return NULL;
	}
	if (!(*pos & 0x80)) {
		/* DMS-100 Service indicator octet - Just ignore for now */
		++pos;
	}
	++pos;

	if (ctrl->debug & PRI_DEBUG_APDU) {
		asn1_dump(ctrl, pos, end);
	}

	pos = fac_dec_extension_header(ctrl, pos, end, header);
	return pos;
}

/*!
 * \brief Decode the facility ie contents for debug purposes.
 *
 * \param ctrl D channel controller for any diagnostic messages.
 * \param buf Buffer containing the facility ie contents.
 * \param length Length of facility ie contents.
 *
 * \return Nothing
 *
 * \note Should only be called if PRI_DEBUG_APDU is enabled.  Otherwise,
 * it it does nothing useful.
 */
void facility_decode_dump(struct pri *ctrl, const unsigned char *buf, size_t length)
{
	const unsigned char *pos;
	const unsigned char *end;
	union {
		struct fac_extension_header header;
		struct rose_message rose;
	} discard;

	end = buf + length;
	pos = facility_decode_header(ctrl, buf, end, &discard.header);
	while (pos && pos < end) {
		pos = rose_decode(ctrl, pos, end, &discard.rose);
	}
}

/* ------------------------------------------------------------------- */
/* end rose.c */
