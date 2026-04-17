/****** Object:  StoredProcedure [datamart].[usp_load_icm_product]    Script Date: 9/29/2025 3:44:49 PM ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

ALTER PROC [datamart].[usp_load_icm_product] @CurrDateTime [datetime] AS

--Declare Needed Variables---------------------------------------------------------------------------------------
DECLARE @CurrDT as datetime, @ChangeDate as date
SET @CurrDT = @CurrDateTime
SET @ChangeDate = CAST(DATEADD(d,-1,@CurrDT) as date)

--Capture Master Product Records---------------------------------------------------------------------------------
IF OBJECT_ID(N'tempdb..#TempProd') IS NOT NULL DROP TABLE #TempProd
SELECT dp.product_id
,dp.product_description
,'PrimoBrands' business_unit
,dp.product_line_description
,dp.category_description
,dp.sub_category_description
,dp.product_type_code
,dp.bottle_type
,dp.brand
,dp.source
,dp.product_group
,dp.product_code
,dp.product_segment
,RTRIM(dp.product_line) product_line
,RTRIM(dp.category_code) category_code
,RTRIM(dp.sub_category_code) sub_category_code
,CASE WHEN quantity_in_package = '187' THEN 78 ELSE 1 END product_uom
INTO #TempProd
FROM datamart.dim_product_segment dp with (nolock)

--Grab Latest product record to compare against-----------------------------------------------------------------
IF OBJECT_ID(N'tempdb..#TempProdPrev') IS NOT NULL DROP TABLE #TempProdPrev
SELECT *
,ROW_NUMBER() OVER (PARTITION BY product_id ORDER BY last_modified_date DESC) RowOrder
INTO #TempProdPrev
FROM datamart.icm_product with (nolock)

;WITH ProductChanges AS (
	SELECT product_id
	,product_description
	,business_unit
	,product_line_description
	,category_description
	,sub_category_description
	,product_type_code
	,bottle_type
	,brand
	,source
	,product_group
	,product_code
	,product_segment
	,product_line
	,category_code
	,sub_category_code
	,product_uom
	FROM #TempProd
EXCEPT
	SELECT product_id
	,product_description
	,business_unit
	,product_line_description
	,category_description
	,sub_category_description
	,product_type_code
	,bottle_type
	,brand
	,source
	,product_group
	,product_code
	,product_segment
	,product_line
	,category_code
	,sub_category_code
	,product_uom
	FROM #TempProdPrev
)

INSERT INTO datamart.icm_product
SELECT product_id
,@ChangeDate last_modified_date
,product_description
,business_unit
,product_line_description
,category_description
,sub_category_description
,product_type_code
,bottle_type
,brand
,source
,product_group
,product_code
,product_segment
,product_line
,category_code
,sub_category_code
,product_uom
FROM ProductChanges
GO