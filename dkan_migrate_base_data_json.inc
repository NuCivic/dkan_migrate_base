<?php

/**
 * @file
 * Migration Class for Datasets.
 */

/**
 * Migrate Data.json datasets.
 */
class MigrateDataJsonDatasetBase extends MigrateDKAN {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $this->endpoint = isset($arguments['endpoint']) ? $arguments['endpoint'] : 'http://demo.getdkan.com/data.json';
    $this->version = isset($arguments['version']) ? $arguments['version'] : '1.1';
    $this->page = isset($arguments['page']) ? $arguments['page'] : '';
    $this->offset = isset($arguments['offset']) ? $arguments['offset'] : '';
    $fields = $this->getDataJsonDatasetFields();

    $this->highwaterField = array(
      'name' => 'modified',
    );
    $this->source = new MigrateSourceList(new DataJsonListJSON(
      $this->endpoint,
      array('page' => $this->page,
            'offset' => $this->offset),
      $this->version),
      new DataJsonItemJSON($this->endpoint, array(), $this->version), $fields);

    $this->map = new MigrateSQLMap(
      $this->machineName,
      array(
        'uuid' => array(
          'type' => 'varchar',
          'length' => 255,
          'not null' => TRUE,
          'description' => 'id',
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    // These are common to both 1.0 and 1.1
    $this->addFieldMapping('title', 'title');
    $this->addFieldMapping('body', 'description');
    $this->addFieldMapping('og_group_ref', 'group_id');
    // We process keywords in prepareRow to make sure taxonomy exists.
    $this->addFieldMapping('field_tags', 'tag_names');
    // $this->addFieldMapping('field_tags', 'keyword');
    $this->addFieldMapping('changed', 'modified');
    $this->addFieldMapping('created', 'issued');
    $this->addFieldMapping('field_public_access_level', 'accessLevel');
    // We will add resources separately.
    // $this->addFieldMapping('field_resources', 'distribution');
    $this->addFieldMapping('field_public_access_level', 'accessLevel');
    $this->addFieldMapping('field_contact_name', 'contactPoint');
    $this->addFieldMapping('field_contact_email', 'mbox');
    $this->addFieldMapping('uuid', 'identifier');
    $this->addFieldMapping('field_spatial', 'spatial');

    if ($this->version == '1.0') {
      $this->addFieldMapping('field_public_access_level', 'accessLevel');
      $this->addFieldMapping('field_additional_info', 'webService');
      $this->addFieldMapping('field_license', 'license');
      $this->addFieldMapping('field_spatial_geographical_cover', 'spatial');
      $this->addFieldMapping('field_temporal', 'temporal');
      $this->addFieldMapping('field_data_dictionary', 'dataDictionary');
      $this->addFieldMapping('field_frequency', 'accrualPeriodicity');
      $this->addFieldMapping('language', 'language');

      // Part of Open Data Federal Extras module.
      // $this->addFieldMapping('', 'bureauCode');
      // $this->addFieldMapping('', 'programCode');

      // Not implemented.
      // $this->addFieldMapping('', 'accessLevelComment');
      // $this->addFieldMapping('', 'theme');
      // $this->addFieldMapping('', 'dataQuality');
      // $this->addFieldMapping('', 'PrimaryITInvestmentUII');
      // $this->addFieldMapping('', 'landingPage');
      // $this->addFieldMapping('', 'systemOfRecords');

      // For resource if only one file.
      // $this->addFieldMapping('', 'accessURL');
      // $this->addFieldMapping('', 'format');
    }
    else {
      // POD 1.1.
      $this->addFieldMapping('field_additional_info:first', 'field_additional_info_key');
      $this->addFieldMapping('field_additional_info:second', 'field_additional_info_value');
      // TODO: Licence.
    }

    $this->destination = new MigrateDestinationNode('dataset', array('text_format' => 'html'));
  }

  /**
   * Creates list of fields for Data.json Dataset.
   */
  public function getDataJsonDatasetFields($version = '1.0') {
    if ($version == '1.0') {
      return array(
        // “Common Core” Required Fields.
        "title" => "Title",
        "description" => "Description",
        "keyword" => "Tags",
        "modified" => "Last Update",
        "publisher" => "Publisher",
        "contactPoint" => "Contact Name",
        "mbox" => "Contact Email",
        "identifier" => "Unique Identifier",
        "accessLevel" => "Public",
        // “Common Core” Required-if-Applicable Fields.
        "bureauCode" => " Bureau Code",
        "programCode" => "Program",
        "accessLevelComment" => " Access Level Comment",
        "accessURL" => "Download",
        "webService" => " Endpoint ",
        "format" => " Format",
        "license" => "License",
        "spatial" => "Spatial",
        "temporal" => " Temporal",
        // Beyond Common Core.
        "theme" => "Category",
        "dataDictionary" => "Data Dictionary",
        "dataQuality" => "Data Quality",
        "distribution" => "Distribution",
        "accrualPeriodicity" => "Frequency",
        "landingPage" => "Homepage",
        "language" => "Language",
        "PrimaryITInvestmentUII" => "Primary IT Investment",
        "references" => "Related Documents",
        "issued" => "Release Date",
        "systemOfRecords" => "System of Records",
        // This doesn't actually exist but we are adding it later in prepareRow.
        "uid" => "User ID",
      );

    }
    else {
      return array(
        "title" => "Title",
        "description" => "Description",
        "keyword" => "Tags",
        "modified" => "Last Update ",
        "publisher" => "Publisher",
        "contactPoint" => " Contact Name and Email",
        "identifier" => " Unique Identifier",
        "accessLevel" => "Public",
        "bureauCodeUSG" => "Bureau",
        "programCodeUSG" => " Program Code ",
        "license" => "License",
        "rights" => " Rights",
        "spatial" => "Spatial",
        "temporal" => " Temporal",
        "distribution" => " Distribution",
        "accrualPeriodicity" => " Frequency",
        "conformsTo" => " Data Standard URI",
        "dataQualityUSG" => " Data Quality",
        "describedBy" => "Data",
        "describedByType" => "Data",
        "isPartOf" => " Collection",
        "issued" => " Release Date",
        "language" => " Language",
        "landingPage" => "Homepage",
        "primaryITInvestmentUIIUSG" => "Primary",
        "references" => " Related Documents ",
        "systemOfRecordsUSG" => " System of Records",
        "theme" => "Theme",
        // This doesn't actually exist but we are adding it later in prepareRow.
        "uid" => "User ID",
      );
    }
  }

  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {
    $tags = taxonomy_vocabulary_machine_name_load('tags');
    if (isset($row->keyword)) {
      foreach ($row->keyword as $tag) {
        $this->createTax($tag, 'tags', $tags->vid);
        $row->tag_names[] = $tag;
      }
    }
    if ($this->version == '1.0') {
    }
    else {
      // POD 1.1.
      if (!$row->group_id = $this->getGroupNidByTitle($row->publisher->name)) {
        if ($row->publisher->name) {
          $group = new stdClass;
          $group->title = $row->publisher->name;
          $group = $this->createGroupNode($group);
          $this->map->saveIDMapping($group, array($group->nid), 0, 0, '');
          $row->group_id = $group->nid;
        }
      }
      $row->mbox = str_replace("mailto:", "", $row->contactPoint->hasEmail);
      $row->contactPoint = $row->contactPoint->fn;
      $uuid = explode("/", $row->uuid);
      $row->identifier = array_pop($uuid);
    }
  }

  /**
   * Implements prepareRow.
   */
  public function prepare($entity, $row) {
    if ($row->distribution) {
      $resource_nids = array();
      foreach ($row->distribution as $key => $dist) {
        $resource_nids[] = $this->createResourceFromDataJson($dist);
      }
    }
    // Link the new resources to the dataset.
    foreach ($resource_nids as $num => $nid) {
      $entity->field_resources['und'][$num]['target_id'] = $nid;
    }
    if ($this->version == '1.0') {
    }
    else {
      // POD 1.1.
      // LandPage and WebService go in additional_fields.
      $add_field_count = 0;
      if (isset($row->landingPage) && $row->landingPage) {
        $entity->field_additional_info['und'][$add_field_count]['first'] = t('Landing Page');
        $entity->field_additional_info['und'][$add_field_count]['second'] = $row->landingPage;
        $add_field_count++;
      }
      if (isset($row->webService) && $row->webService) {
        $entity->field_additional_info['und'][$add_field_count]['first'] = t('Web Service');
        $entity->field_additional_info['und'][$add_field_count]['second'] = $row->webService;
      }
      // Converts geojson spatial for saving field.
      if (isset($entity->field_spatial['und'][0]['wkt']) && $spatial = $entity->field_spatial['und'][0]['wkt']) {
        $spatial = explode(',', $spatial);
        $count = 0;
        $point= '';
        foreach ($spatial as $key => $val) {
          if ($key && ($key & 1)) {
            $point = 'POINT (' . implode(" ", array($spatial[$key-1], $spatial[$key])) . ')';
            //geophp_load();
            //$geometry = geoPHP::load($point, 'wkt');
            //$entity->field_spatial['und'][$count] = geofield_get_values_from_geometry($geometry);
            $count++;
            $point = '';
          }
        }
      }
    }
  }

  /**
   * Creates and maps resource from data.json source.
   *
   * Dist includes:
   *  - title
   *  - format
   *  - downloadURL (1.1) or accessURL (1.0).
   */
  public function createResourceFromDataJson($dist) {
    $resource_url = '';
    global $user;
    if (isset($dist->accessURL)) {
      $resource_url = $dist->accessURL;
    }
    elseif (isset($dist->downloadURL)) {
      $resource_url = $dist->downloadURL;
    }
    // Creates a new resource for every linked file.
    // Linked files contain title, format, and accessURL.
    $node = new stdClass();
    $node->is_new = TRUE;
    $node->language = 'und';
    $node->uid = $user->uid;
    $node->type = 'resource';
    $node->title = isset($dist->title) ? $dist->title : $dist->format;
    $dist->format = strtolower($dist->format);
    $format = taxonomy_vocabulary_machine_name_load('format');
    $term = $this->createTax($dist->format, 'format', $format->vid);
    $node->field_format['und'][0]['tid'] = $term->tid;
    $response = drupal_http_request($resource_url);
    if ($response->code == '200') {
      $file = remote_stream_wrapper_file_load_by_uri($resource_url);
      // Create a link to file for certain formats.
      if ($dist->format == 'csv'
        || $dist->format == 'geojson'
        || $dist->format == 'kml'
        || $dist->format == 'zip') {
        if (!$file) {
          $file = remote_stream_wrapper_file_create_by_uri($resource_url);
          $file->status = FALSE;
          file_save($file);
        }
        $node->field_link_remote_file['und'][0]['fid'] = $file->fid;
        $node->field_link_remote_file['und'][0]['display'] = 1;
      }
      else {
        // Link to API for all others.
        $node->field_link_api['und'][0]['url'] = $resource_url;
      }
    }
    else {
      // Link to API if "not found".
      $node->field_link_api['und'][0]['url'] = $resource_url;
    }
    node_save($node);
    // Adds the node to the map. This means that when the migration is
    // rolled back that the resources are removed as well. This adds the
    // newly created uuids and nids to the migrate_map_data_json table.
    $this->map->saveIDMapping($node, array($node->nid), 0, 0, '');
    return $node->nid;
  }
}

class DataJsonListJSON extends MigrateListJSON {

  /**
   * Adds version to class.
   */
  public function __construct($list_url, $http_options = array(), $version = '1.0') {
    parent::__construct($list_url);
    $this->listUrl = $list_url;
    $this->httpOptions = $http_options;
    $this->page = isset($http_options['page']) ? $http_options['page'] : '';
    $this->offset = isset($http_options['offset']) ? $http_options['offset'] : '';
    $this->version = $version;
  }

  /**
   * Implements computeCount().
   */
  public function computeCount() {
    $count = 0;
    if (!filter_var($this->listUrl, FILTER_VALIDATE_URL)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    if ($json) {
      $data = drupal_json_decode($json);
      if ($this->version == '1.1') {
        $datasets = 0;
        foreach ($data['dataset'] as $item) {
          // Adds resources to count.
          if (isset($item['distribution'])) {
            $count++;
            $count = $count + count($item['distribution']);
          }
          if (isset($item['publisher']['name'])) {
            $count++;
          }
          $datasets++;
          if ($this->page && $datasets >= $this->page) {
            break;
          }
        }
      }
      else {
        foreach ($data as $item) {
          // Adds resources to count.
          if (isset($item['distribution'])) {
            $count++;
            $count = $count + count($item['distribution']);
          }
        }
      }
    }
    if ($this->version == '1.1') {
      $dataset_count = $this->page ? $this->page : count($data['dataset']);
    }
    else {
      $dataset_count = $this->page ? $this->page : count($data);
    }
    $resource_count = $count - $dataset_count;
    drupal_set_message(t("%url contains %datasets datasets and %resources resources.", array('%datasets' => $dataset_count, '%url' => $this->listUrl, '%resources' => $resource_count )));
    return $count;
  }

  public function getIdList() {
    migrate_instrument_start("Retrieve $this->listUrl");
    if (!filter_var($this->listUrl, FILTER_VALIDATE_URL)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    migrate_instrument_stop("Retrieve $this->listUrl");
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        return $this->getIDsFromJSON($data);
      }
    }
    Migration::displayMessage(t('Loading of !listurl failed:',
        array('!listurl' => $this->listUrl)));
    return NULL;
  }

  /**
   * Retrieves ID from list.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    if ($this->version == '1.0') {
      foreach ($data as $item) {
        $ids[] = $item['identifier'];
      }
    }
    else {
      $datasets = 0;
      $total = $this->page + $this->offset;
      foreach ($data['dataset'] as $item) {
        if ($datasets < $this->offset) {
          $datasets++;
          continue;
        }
        $uuid = explode("\\", $item['identifier']);
        $ids[] = array_pop($uuid);
        $datasets++;
        if ($total && $datasets >= $total) {
          break;
        }
      }
    }
    return $ids;
  }
}

class DataJsonItemJSON extends MigrateItemJSON {
  protected $data = array();
  /**
   * Adds version to class.
   */
  public function __construct($list_url, $http_options = array(), $version = '1.0') {
    parent::__construct($list_url);
    $this->listUrl = $list_url;
    $this->httpOptions = $http_options;
    $this->version = $version;
  }

  /**
   * Retrieves from itemUrl.
   */
  public function getItem($id) {
    // We cache the parsed JSON at $this->data.
    if (empty($this->data)) {
      $data = $this->loadJSONUrl($this->itemUrl);
      if ($data) {
        if ($this->version == '1.0') {
          foreach ($data as $item) {
            $this->data[$item->identifier] = $item;
          }
        }
        else {
          foreach ($data->dataset as $item) {
            $this->data[$item->identifier] = $item;
          }
        }
      }
      else {
        // TODO: Proper error handling.
      }
    }
    // Return the requested item.
    if (isset($this->data[$id])) {
      return $this->data[$id];
    }
    else {
      return NULL;
    }
  }
}
