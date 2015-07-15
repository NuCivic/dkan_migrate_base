<?php
class DKANMigrateBaseTest  extends PHPUnit_Framework_TestCase
{

    protected function setUp()
    {
        migrate_static_registration();
    }

    public function getNodeByTitle($title) {
      $query = new EntityFieldQuery();

       $entities = $query->entityCondition('entity_type', 'node')
        ->propertyCondition('title', $title)
        ->range(0,1)
        ->execute();

        if (!empty($entities['node'])) {
          $node = node_load(array_shift(array_keys($entities['node'])));
        }
        return $node;
    }

    public function ResourceAssert($expected, $result)
    {
      foreach ($expected as $field => $value) {
        $this->assertEquals($expected[$field], $result[$field]);
      }

    }

    public function rollback($migrationName, $remainingNodes = 4)
    {
      $migration = Migration::getInstance($migrationName);
      $result = $migration->processRollback();
      // Test rollback
      // TODO: DKAN comes with 4 resources. Remove first so count is 0.
      $rawnodes = node_load_multiple(FALSE, array('type' => 'dataset'), TRUE);
      $this->assertEquals($remainingNodes, count($rawnodes));
      $count = db_select('migrate_map_' . $migrationName, 'map')
                ->fields('map', array('sourceid1'))
                ->countQuery()
                ->execute()
                ->fetchField();
      $this->assertEquals(0, $count);
    }

    public function migrate($migrationName)
    {
      // Run migration.
      $migration = Migration::getInstance($migrationName);
      $result = $migration->processImport();
      $this->assertEquals(Migration::RESULT_COMPLETED, $result);
      $this->assertEquals(0, $migration->errorCount());
    }

    public function testCKANResourceImport()
    {
      $expected = $result = array();
      $this->migrate('dkan_migrate_base_example_ckan_resources');

      $node = $this->getNodebyTitle('Madison Polling Places Test');
      $file = $node->field_link_remote_file['und'][0];
      $body = 
      '<p>This is a list and map of polling places in Madison, WI.</p>

<p>Original data here: 
  <a href="https://data.cityofmadison.com/Polling-Places/Polling-Places/rtyh-6ucr">https://data.cityofmadison.com/Polling-Places/Polling-Places/rtyh-6ucr</a></p>';
      $format = taxonomy_term_load($node->field_format['und'][0]['tid']);

      $result['title']  = $node->title;
      $expect['title']  = "Madison Polling Places Test";
      $result['body']   = $node->body['und'][0]['value'];
      $expect['body']   = $body;
      $result['format'] = $format->name;
      $expect['format'] = 'csv';
      $result['file']   = substr($file['filename'], 0, 22);
      $expect['file']   = 'Polling_Places_Madison';

      $this->resourceAssert($expect, $result);
    }

    public function testCKANDatasetImport()
    {
      $expected = $result = array();
      $this->migrate('dkan_migrate_base_example_ckan_dataset');

      $node = $this->getNodebyTitle('Wisconsin Polling Places Test');

      $result['title']    = $node->title;
      $expect['title']    = "Wisconsin Polling Places Test";
      $result['created']  = $node->created;
      $expect['created']  = "1360559580";
      $result['id']       = $node->uuid;
      $expect['id']       = "eabaa139-4d2c-4ecf-b81e-cad681c3212e";
      // TODO:
      // maintainer
      // maintainer_email
      // licence_title
      
      $this->resourceAssert($expect, $result);
    }

    public function testCKANDatasetRollback() {
      $this->rollback('dkan_migrate_base_example_ckan_dataset');
    }

    public function testCKANResourceRollback() {
      $this->rollback('dkan_migrate_base_example_ckan_resources');
    }

    public function testCKANDataJsonImport()
    {
      $expected = $result = array();
      $this->migrate('dkan_migrate_base_example_data_json11');

      $node = $this->getNodebyTitle('Gross Rent over time');
      $group = isset($node->og_group_ref['und'][0]['target_id']) ? node_load($node->og_group_ref['und'][0]['target_id']) : NULL;
      $keyword1 = taxonomy_term_load($node->field_tags['und'][0]['tid']);
      $keyword2 = taxonomy_term_load($node->field_tags['und'][1]['tid']);
      $resource1 = node_load($node->field_resources['und'][0]['target_id']);
      $resource2 = node_load($node->field_resources['und'][1]['target_id']);
      $format1 = taxonomy_term_load($resource1->field_format['und'][0]['tid']);
      $format2 = taxonomy_term_load($resource2->field_format['und'][0]['tid']);

      $result['title']  = $node->title;
      $expect['title']  = "Gross Rent over time";
      $result['id']  = $node->uuid;
      $expect['id']  = "b6a4942e-fa73-4cbf-804f-1f9eea6d02df";
      $result['keyword1']  = $keyword1->name;
      $expect['keyword1']  = "housing";
      $result['keyword1']  = $keyword2->name;
      $expect['keyword1']  = "rent";
      $result['group']  = $group->title;
      $expect['group']  = "Housing";
      $result['license']  = $node->field_license['und'][0]['value'];
      $expect['license']  = "notspecified";
      $result['modified']  = date('m d y', $node->changed);
      $expect['modified']  = "06 24 14";
      $result['accessLevel']  = $node->field_public_access_level['und'][0]['value'];
      $expect['accessLevel']  = "public";
      $result['contactPoint']  = $node->field_contact_name['und'][0]['value'];
      $expect['contactPoint']  = "Bruce Wayne";
      $result['contactEmail']  = $node->field_contact_email['und'][0]['value'];
      $expect['contactEmail']  = "bruce@notbatman.com";

      $result['resource1Name']  = $resource1->title;
      $expect['resource1Name']  = "txt";
      $result['resource1Format']  = $format1->name;
      $expect['resource1Format']  = "txt";
      $result['resource1DownloadUrl']  = $resource1->field_link_api['und'][0]['url'];
      $expect['resource1DownloadUrl']  = "http://example.com/sites/default/files/grossrents_adj.txt";

      $result['resource2Name']  = $resource2->title;
      $expect['resource2Name']  = "csv";
      $result['resource2Format']  = $format2->name;
      $expect['resource2Format']  = "csv";
      $result['resource2DownloadUrl']  = $resource2->field_link_api['und'][0]['url'];
      $expect['resource2DownloadUrl']  = "http://example.com/sites/default/files/grossrents_adj.csv";

      // TODO:
      // maintainer
      // maintainer_email
      // licence_title

      $this->resourceAssert($expect, $result);
    }

    public function testCKANDataJsonRollback() {
      $this->rollback('dkan_migrate_base_example_data_json11');
    }

}
